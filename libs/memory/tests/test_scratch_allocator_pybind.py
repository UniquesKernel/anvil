"""Stateful Hypothesis tests validating the scratch allocator bindings (simplified)."""

import ctypes
from ctypes import c_size_t, c_void_p
from typing import List, Tuple, Dict

import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition, invariant
from hypothesis.strategies import integers, binary

import anvil_memory as am

# Records: (address:int, size:int, alignment:int)
AllocationRecord = Tuple[int, int, int]
PTR_ALIGN = ctypes.sizeof(ctypes.c_void_p)

def align_up(address: int, alignment: int) -> int:
    """Smallest aligned address >= address."""
    rem = address % alignment
    return address if rem == 0 else address + (alignment - rem)


@hypothesis.settings(
    max_examples=1000,
)
class ScratchAllocatorModel(RuleBasedStateMachine):
    """Rule-based model exercising scratch allocator behavior."""

    def __init__(self):
        super().__init__()
        self.allocator = None
        self.capacity = 0
        self.allocations: List[AllocationRecord] = []
        # Map: address(int) -> original bytes
        self.copied_data: Dict[int, bytes] = {}

    @rule(
        exponent=integers(min_value=am.MIN_ALIGNMENT_EXPONENT, max_value=am.MAX_ALIGNMENT_EXPONENT),
        capacity=integers(min_value=64, max_value=(1 << 20)),
    )
    @precondition(lambda self: self.allocator is None)
    def create_scratch_allocator(self, exponent: int, capacity: int):
        alignment = 1 << exponent
        self.allocator = am.scratch_allocator_create(capacity, alignment)
        self.capacity = capacity
        self.allocations.clear()
        self.copied_data.clear()

    @rule()
    @precondition(lambda self: self.allocator is not None)
    def destroy(self):
        err = am.scratch_allocator_destroy(self.allocator)
        self.allocator = None
        self.capacity = 0
        self.allocations.clear()
        self.copied_data.clear()
        assert err == am.ERR_SUCCESS, f"Allocator destruction failed with error code {err}"

    @rule()
    @precondition(lambda self: self.allocator is not None)
    def allocator_reset(self):
        err = am.scratch_allocator_reset(self.allocator)
        self.allocations.clear()
        self.copied_data.clear()
        assert err == am.ERR_SUCCESS, f"Allocator reset failed with error code {err}"

    @rule(
        alloc_size=integers(min_value=1, max_value=(1 << 20)),
        exponent=integers(min_value=am.MIN_ALIGNMENT_EXPONENT, max_value=am.MAX_ALIGNMENT_EXPONENT),
    )
    @precondition(lambda self: self.allocator is not None)
    def alloc(self, alloc_size: int, exponent: int):
        alignment = 1 << exponent
        ptr = am.scratch_allocator_alloc(self.allocator, alloc_size, alignment)
        if ptr:
            addr = am.ptr_to_int(ptr)
            self.allocations.append((addr, alloc_size, alignment))

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) >= 1)
    def inv_no_alloc_overlap(self):
        for i, (a1, s1, _) in enumerate(self.allocations):
            for a2, s2, _ in self.allocations[i + 1 :]:
                if a1 < a2 + s2 and a2 < a1 + s1:
                    assert False, f"Overlap: [{a1},{a1+s1}) with [{a2},{a2+s2})"

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) >= 1)
    def inv_allocations_are_contiguous(self):
        for i, (addr, size, _) in enumerate(self.allocations[:-1]):
            next_addr, _, next_align = self.allocations[i + 1]
            expected = align_up(addr + size, next_align)
            assert expected == next_addr, f"Not contiguous: expected {expected}, got {next_addr}"

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) > 0)
    def inv_allocations_properly_aligned(self):
        for addr, _, align in self.allocations:
            assert addr % align == 0, f"Address {addr} not aligned to {align}"

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) > 0)
    def inv_allocations_within_bounds(self):
        first_addr = self.allocations[0][0]
        last_addr, last_size, _ = self.allocations[-1]
        used = (last_addr + last_size) - first_addr
        assert used <= self.capacity, f"Used {used} > capacity {self.capacity}"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_power_of_two_alignments(self):
        for _, _, align in self.allocations:
            assert align > 0 and (align & (align - 1)) == 0, f"Alignment {align} not power-of-two"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_positive_sizes(self):
        for _, size, _ in self.allocations:
            assert size > 0, f"Invalid allocation size: {size}"

TestMyStateMachine = ScratchAllocatorModel.TestCase  
