from anvil_memory.lazy_scratch_allocator import (
    lazy_scratch_allocator_alloc,
    lazy_scratch_allocator_create,
    lazy_scratch_allocator_destroy,
    lazy_scratch_allocator_read,
    lazy_scratch_allocator_reset,
    lazy_scratch_allocator_write,
)
from anvil_memory.scratch_allocator import (
    scratch_allocator_alloc,
    scratch_allocator_create,
    scratch_allocator_destroy,
    scratch_allocator_read,
    scratch_allocator_reset,
    scratch_allocator_write,
)
from anvil_memory import Error, MAX_ALIGNMENT, MIN_ALIGNMENT, ptr_to_int
from hypothesis import strategies as st
from hypothesis.stateful import (
    RuleBasedStateMachine,
    initialize,
    invariant,
    precondition,
    rule,
)


def align_up(address, alignment):
    """Smallest aligned address >= address (pure, side-effect free)."""
    if alignment == 0:
        return address
    r = address % alignment
    return address if r == 0 else address + (alignment - r)


def pairwise(xs):
    return list(zip(xs, xs[1:]))


@st.composite
def allocator_config(draw):
    """Generate (capacity, alignment) where alignment is power-of-two in supported range."""
    base = draw(st.integers(min_value=3, max_value=11))
    alignment = 2**base

    capacity = draw(st.integers(min_value=1, max_value=2**24))
    return (capacity, alignment)


class DifferentialScratchModel(RuleBasedStateMachine):
    """Differential testing: lazy-scratch and scratch allocators should behave identically."""

    def __init__(self):
        super().__init__()
        self.lazy_allocator = None
        self.scratch_allocator = None
        self.isValid = False
        self.capacity = 0
        self.alignment = 0
        self.lazy_allocations = []
        self.scratch_allocations = []

    @initialize(config=allocator_config())
    def initialize_model(self, config):
        capacity, alignment = config

        lazy_err, lazy_allocator = lazy_scratch_allocator_create(capacity, alignment)
        scratch_err, scratch_allocator = scratch_allocator_create(capacity, alignment)

        if lazy_err != Error.OK or lazy_allocator is None:
            self.isValid = False
            return

        if scratch_err != Error.OK or scratch_allocator is None:
            self.isValid = False
            return

        assert lazy_err == scratch_err
        assert (lazy_allocator is None) == (scratch_allocator is None)

        self.lazy_allocator = lazy_allocator
        self.scratch_allocator = scratch_allocator
        self.isValid = True
        self.capacity = capacity
        self.alignment = alignment
        self.lazy_allocations.clear()
        self.scratch_allocations.clear()

    @rule(config=allocator_config())
    @precondition(lambda self: self.isValid == False)
    def create(self, config):
        capacity, alignment = config

        lazy_err, lazy_allocator = lazy_scratch_allocator_create(capacity, alignment)
        scratch_err, scratch_allocator = scratch_allocator_create(capacity, alignment)

        if lazy_err != Error.OK or lazy_allocator is None:
            self.isValid = False
            return

        if scratch_err != Error.OK or scratch_allocator is None:
            self.isValid = False
            return

        assert lazy_err == scratch_err
        assert (lazy_allocator is not None) == (scratch_allocator is not None)

        self.lazy_allocator = lazy_allocator
        self.scratch_allocator = scratch_allocator
        self.isValid = True
        self.capacity = capacity
        self.alignment = alignment
        self.lazy_allocations.clear()
        self.scratch_allocations.clear()

    @rule(config=allocator_config())
    @precondition(
        lambda self: (self.lazy_allocator is not None)
        and (self.scratch_allocator is not None)
    )
    def alloc(self, config):
        allocation_size, alignment = config

        lazy_ptr, _ = lazy_scratch_allocator_alloc(
            self.lazy_allocator, allocation_size, alignment
        )
        scratch_ptr, _ = scratch_allocator_alloc(
            self.scratch_allocator, allocation_size, alignment
        )

        assert (lazy_ptr is None) == (scratch_ptr is None)

        if lazy_ptr and scratch_ptr:
            self.lazy_allocations.append((lazy_ptr, allocation_size, alignment))
            self.scratch_allocations.append((scratch_ptr, allocation_size, alignment))

    @rule()
    @precondition(
        lambda self: (self.lazy_allocator is not None)
        and (self.scratch_allocator is not None)
    )
    def allocator_reset(self):
        lazy_err = lazy_scratch_allocator_reset(self.lazy_allocator)
        scratch_err = scratch_allocator_reset(self.scratch_allocator)

        assert lazy_err == scratch_err

        if self.isValid:
            assert lazy_err == Error.OK
            self.lazy_allocations.clear()
            self.scratch_allocations.clear()
        else:
            assert lazy_err == Error.NULL_PARAMETER

    @rule(data=st.data())
    @precondition(lambda self: self.isValid and len(self.lazy_allocations) > 0)
    def write_to_allocation(self, data):
        idx = data.draw(
            st.integers(min_value=0, max_value=len(self.lazy_allocations) - 1)
        )
        lazy_ptr, allocation_size, _ = self.lazy_allocations[idx]
        scratch_ptr, _, _ = self.scratch_allocations[idx]

        payload = bytes(i & 0xFF for i in range(allocation_size))

        lazy_err = lazy_scratch_allocator_write(lazy_ptr, payload)
        scratch_err = scratch_allocator_write(scratch_ptr, payload)
        assert lazy_err == Error.OK
        assert scratch_err == Error.OK

        lazy_read_err, lazy_result = lazy_scratch_allocator_read(lazy_ptr, allocation_size)
        scratch_read_err, scratch_result = scratch_allocator_read(scratch_ptr, allocation_size)
        assert lazy_read_err == Error.OK
        assert scratch_read_err == Error.OK
        assert lazy_result == payload, "Lazy scratch write/read mismatch"
        assert scratch_result == payload, "Scratch write/read mismatch"

    @rule()
    @precondition(
        lambda self: (self.lazy_allocator is not None)
        and (self.scratch_allocator is not None)
    )
    def destroy(self):
        lazy_err = lazy_scratch_allocator_destroy(self.lazy_allocator)
        scratch_err = scratch_allocator_destroy(self.scratch_allocator)

        assert lazy_err == scratch_err

        self.isValid = False
        self.capacity = 0
        self.alignment = 0
        self.lazy_allocations.clear()
        self.scratch_allocations.clear()

    @invariant()
    def inv_same_allocation_count(self):
        assert len(self.lazy_allocations) == len(self.scratch_allocations)

    @invariant()
    @precondition(lambda self: len(self.lazy_allocations) > 0)
    def inv_same_allocation_properties(self):
        assert all(
            lazy_size == scratch_size and lazy_align == scratch_align
            for (_, lazy_size, lazy_align), (_, scratch_size, scratch_align) in zip(
                self.lazy_allocations, self.scratch_allocations
            )
        )

    @invariant()
    @precondition(lambda self: len(self.lazy_allocations) >= 2)
    def inv_same_memory_layout(self):
        lazy_base = ptr_to_int(self.lazy_allocations[0][0])
        scratch_base = ptr_to_int(self.scratch_allocations[0][0])

        assert all(
            (ptr_to_int(lazy_ptr) - lazy_base) == (ptr_to_int(scratch_ptr) - scratch_base)
            for (lazy_ptr, _, _), (scratch_ptr, _, _) in zip(
                self.lazy_allocations, self.scratch_allocations
            )
        )

    @invariant()
    def inv_aligned(self):
        for (ptr, _, align) in self.lazy_allocations:
            assert ptr_to_int(ptr) % align == 0
            assert align > 0
            assert (align & (align - 1)) == 0
            assert MIN_ALIGNMENT <= align <= MAX_ALIGNMENT

    @invariant()
    @precondition(lambda self: len(self.lazy_allocations) >= 2)
    def inv_contiguous_adjacent(self):
        for (ptr_a, size_a, _), (ptr_b, _, align_b) in pairwise(self.lazy_allocations):
            addr_a = ptr_to_int(ptr_a)
            addr_b = ptr_to_int(ptr_b)
            expected_b = align_up(addr_a + size_a, align_b)
            assert addr_b == expected_b, (
                f"Allocations not contiguous: addr={addr_a} size={size_a} "
                f"aligned to {align_b} = {expected_b}, but next starts at {addr_b}"
            )

    @invariant()
    @precondition(lambda self: self.isValid and len(self.lazy_allocations) >= 1)
    def inv_within_capacity_span(self):
        first_ptr, _, _ = self.lazy_allocations[0]
        last_ptr, last_size, _ = self.lazy_allocations[-1]

        first_start = ptr_to_int(first_ptr)
        last_end = ptr_to_int(last_ptr) + last_size
        assert (last_end - first_start) <= self.capacity, (
            f"Used {(last_end - first_start)} > capacity {self.capacity}"
        )


TestDifferentialScratchModel = DifferentialScratchModel.TestCase
