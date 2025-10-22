"""Stateful Hypothesis tests validating the stack allocator (random EAGER/LAZY)."""

import anvil_memory as am
from dataclasses import dataclass
from typing import List, Tuple, Optional

import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition, invariant
from hypothesis.strategies import integers, sampled_from

# --- Helpers -----------------------------------------------------------------

def align_up(address: int, alignment: int) -> int:
    r = address % alignment
    return address if r == 0 else address + (alignment - r)

def to_int(ptr: object) -> int:
    return am.ptr_to_int(ptr)

@dataclass
class Allocation:
    addr: object
    size: int
    alignment: int
    epoch: int

RecordSnapshot = Tuple[int]  # stores allocation_count only

@hypothesis.settings(
    max_examples=1000,
)
class StackAllocatorModel(RuleBasedStateMachine):
    """Exercises allocator lifecycles with mode randomized (EAGER or LAZY)."""

    def __init__(self):
        super().__init__()
        self.allocator = None
        self.capacity = 0
        self.alloc_mode = am.EAGER

        self.allocations: List[Allocation] = []
        self.stack: List[RecordSnapshot] = []
        self.top_ptr: Optional[int] = None
        self.allocation_epoch = 0

    def teardown(self):
        if self.allocator is not None:
            am.stack_allocator_destroy(self.allocator)
        self.allocator = None
        self.allocations.clear()
        self.stack.clear()
        self.top_ptr = None
        self.allocation_epoch = 0
        self.capacity = 0

    @rule(
        exponent=integers(min_value=am.MIN_ALIGNMENT_EXPONENT, max_value=am.MAX_ALIGNMENT_EXPONENT),
        capacity=integers(min_value=64, max_value=(1 << 20)),
        alloc_mode=sampled_from([am.EAGER, am.LAZY]),
    )
    @precondition(lambda self: self.allocator is None)
    def create_stack_allocator(self, exponent: int, capacity: int, alloc_mode: int):
        alignment = 1 << exponent
        self.allocator = am.stack_allocator_create(capacity, alignment, alloc_mode)
        self.capacity = capacity
        self.alloc_mode = alloc_mode

        self.allocations.clear()
        self.stack.clear()
        self.top_ptr = None
        self.allocation_epoch = 0

    @rule()
    @precondition(lambda self: self.allocator is not None)
    def destroy(self):
        err = am.stack_allocator_destroy(self.allocator)
        self.allocator = None

        self.allocations.clear()
        self.stack.clear()
        self.top_ptr = None
        self.capacity = 0
        self.allocation_epoch = 0

        assert err == am.ERR_SUCCESS, f"Allocator destruction failed with error code {err}"

    @rule(
        alloc_size=integers(min_value=1, max_value=(1 << 20)),
        exponent=integers(min_value=am.MIN_ALIGNMENT_EXPONENT, max_value=am.MAX_ALIGNMENT_EXPONENT),
    )
    @precondition(lambda self: self.allocator is not None)
    def alloc(self, alloc_size: int, exponent: int):
        alignment = 1 << exponent
        ptr = am.stack_allocator_alloc(self.allocator, alloc_size, alignment)

        if ptr:
            if self.top_ptr is not None:
                expected = align_up(self.top_ptr, alignment)
                assert to_int(ptr) == expected, f"Expected {expected}, got {to_int(ptr)}"
                self.top_ptr = None

            self.allocations.append(Allocation(ptr, alloc_size, alignment, self.allocation_epoch))

    @rule()
    @precondition(lambda self: self.allocator is not None)
    def allocator_reset(self):
        err = am.stack_allocator_reset(self.allocator)
        self.allocations.clear()
        self.stack.clear()
        self.top_ptr = None
        self.allocation_epoch = 0
        assert err == am.ERR_SUCCESS, f"Allocator reset failed with error code {err}"

    @rule()
    @precondition(lambda self: self.allocator is not None and 1 <= len(self.allocations) <= 50)
    def record(self):
        am.stack_allocator_record(self.allocator)
        self.stack.append((len(self.allocations),))

    @rule()
    @precondition(lambda self: self.allocator is not None and len(self.stack) > 0)
    def unwind(self):
        am.stack_allocator_unwind(self.allocator)
        (allocation_count,) = self.stack.pop()
        self.allocations = self.allocations[:allocation_count]
        self.allocation_epoch += 1

        if self.allocations:
            last = self.allocations[-1]
            self.top_ptr = to_int(last.addr) + last.size
        else:
            self.top_ptr = None

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) >= 1)
    def inv_no_alloc_overlap(self):
        ordered = sorted(self.allocations, key=lambda a: to_int(a.addr))
        for prev, curr in zip(ordered, ordered[1:]):
            prev_end = to_int(prev.addr) + prev.size
            assert prev_end <= to_int(curr.addr), (
                f"Overlap: [{to_int(prev.addr)}, {prev_end}) with start {to_int(curr.addr)}"
            )

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) >= 2)
    def inv_allocations_are_contiguous_within_epoch(self):
        for a, b in zip(self.allocations, self.allocations[1:]):
            if a.epoch == b.epoch:
                a_end = to_int(a.addr) + a.size
                expected = align_up(a_end, b.alignment)
                assert to_int(b.addr) >= expected, (
                    f"Non-contiguous in epoch {a.epoch}: expected >= {expected}, got {to_int(b.addr)}"
                )

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) > 0)
    def inv_allocations_properly_aligned(self):
        for a in self.allocations:
            assert to_int(a.addr) % a.alignment == 0, (
                f"Address {to_int(a.addr)} not aligned to {a.alignment}"
            )

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) > 0)
    def inv_allocations_within_bounds(self):
        first = self.allocations[0]
        last = self.allocations[-1]
        total_used = (to_int(last.addr) + last.size) - to_int(first.addr)
        assert total_used <= self.capacity, f"Used {total_used} bytes exceeds capacity {self.capacity}"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_power_of_two_alignments(self):
        for a in self.allocations:
            assert a.alignment > 0 and (a.alignment & (a.alignment - 1)) == 0, (
                f"Alignment {a.alignment} is not a power of 2"
            )

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_positive_sizes(self):
        for a in self.allocations:
            assert a.size > 0, f"Invalid allocation size: {a.size}"

TestMyStateMachine = StackAllocatorModel.TestCase
