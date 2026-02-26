from anvil_memory.stack_allocator import stack_allocator_create, stack_allocator_destroy, stack_allocator_alloc, stack_allocator_reset, stack_allocator_record, stack_allocator_unwind
from anvil_memory import Error, MAX_STACK_DEPTH, ptr_to_int, MAX_ALIGNMENT, MIN_ALIGNMENT
from hypothesis.stateful import RuleBasedStateMachine, rule, initialize, precondition, invariant
from hypothesis import strategies as st

def pairwise(xs):
    return list(zip(xs, xs[1:]))

def align_up(address, alignment):
    """Smallest aligned address >= address (pure, side-effect free)."""
    if alignment == 0:
        return address
    r = address % alignment
    return address if r == 0 else address + (alignment - r)

@st.composite
def allocator_config(draw):
    """Generate (capacity, alignment) where capacity >= alignment"""
    base = draw(st.integers(min_value=3, max_value=11))
    alignment = 2 ** base  
    
    capacity = draw(st.integers(min_value=1, max_value=2**24))
    return (capacity, alignment)

class StackAllocatorModel(RuleBasedStateMachine):
    def __init__(self):
        super().__init__()
        self.allocator = None
        self.isValid = False
        self.capacity = 0
        self.allocations = []
        self.stack = []
        self.epoch = 0
        self.allocated = 0

    @initialize(config=allocator_config())
    def initialize_model(self, config):
        capacity, alignment = config

        err, allocator = stack_allocator_create(capacity,alignment)

        if err != Error.OK or allocator is None:
            self.isValid = False
            return

        assert allocator is not None
        assert err == Error.OK

        self.allocator = allocator
        self.isValid = True
        self.capacity = capacity
        self.allocations.clear()
        self.stack.clear()
        self.epoch = 0
        self.allocated = 0

    @rule(config = allocator_config())
    @precondition(lambda self: self.isValid == False)
    def create(self, config):
        capacity, alignment = config
        err, allocator = stack_allocator_create(capacity, alignment)

        if err != Error.OK or allocator is None:
            self.isValid = False
            return

        assert allocator is not None
        assert err == Error.OK

        self.allocator = allocator
        self.isValid = True
        self.capacity = capacity
        self.allocations.clear()
        self.stack.clear()
        self.epoch = 0
        self.allocated = 0

    @rule(config = allocator_config())
    @precondition(lambda self: self.allocator is not None)
    def alloc(self, config):
        allocation_size, alignment = config
        ptr, _ = stack_allocator_alloc(self.allocator, allocation_size, alignment)

        if self.isValid == False:
            assert ptr is None, "Can't allocate from invalid allocator"
            return

        if ptr:
            self.allocations.append((ptr, allocation_size, alignment, self.epoch))
            self.allocated = self.allocated + allocation_size

    @rule()
    @precondition(lambda self: self.allocator is not None)
    def allocator_reset(self):
        err = stack_allocator_reset(self.allocator)
        self.allocations.clear()
        self.stack.clear()
        self.allocation_epoch = 0
        self.allocated = 0

        if (self.isValid == True):
            assert err == Error.OK, f"Allocator reset failed with error code {err}"
        else:
            assert err == Error.NULL_PARAMETER


    @rule()
    @precondition(lambda self: self.allocator is not None)
    def record(self):
        err = stack_allocator_record(self.allocator)
        
        if (self.isValid == False):
            assert err == Error.NULL_PARAMETER
            return

        if (len(self.allocations) > MAX_STACK_DEPTH):
            assert err == Error.INVALID_ARGUMENT
        else: 
            assert err == Error.OK
            epoch_alloc_size = sum([x for (_, x, _, epoch) in self.allocations if epoch == self.epoch])
            self.stack.append((len(self.allocations), epoch_alloc_size))
            self.epoch += 1

    @rule()
    @precondition(lambda self: self.allocator is not None)
    def unwind(self):
        err = stack_allocator_unwind(self.allocator)
    
        if (self.isValid == False):
            assert err == Error.NULL_PARAMETER
            return

        if (len(self.stack) == 0):
            assert err == Error.INVALID_ARGUMENTS
            return

        allocation_count, epoch_size = self.stack.pop()
        self.allocations = self.allocations[:allocation_count]
        self.epoch -= 1
        self.allocated -= epoch_size

    @rule()
    @precondition(lambda self: self.allocator is not None)
    def destroy(self): 
        err = stack_allocator_destroy(self.allocator)
        if (self.isValid == True):
            assert err == Error.OK
        else:
            assert err == Error.NULL_PARAMETER

        self.isValid = False

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_no_alloc_overlap(self):
        ordered = sorted(self.allocations, key=lambda alloc: ptr_to_int(alloc[0]))
        intervals = [(ptr_to_int(ptr_a), ptr_to_int(ptr_a) + allocation_size) for (ptr_a, allocation_size, _, _) in ordered]
        assert all(prev[1] <= curr[0] for prev, curr in zip(intervals, intervals[1:])), (
            f"Overlapping intervals detected: {intervals}"
        )

    @invariant()
    @precondition(lambda self: len(self.allocations) >= 2)
    def inv_contiguous_within_epoch(self):
        assert all(
            ptr_to_int(ptr_b) == align_up(ptr_to_int(ptr_a) + size_a, align_b)
            for (ptr_a, size_a, _, epoch_a), (ptr_b, _, align_b, epoch_b) in zip(self.allocations, self.allocations[1:])
            if epoch_a == epoch_b
        ), "Adjacent allocations within an epoch must be contiguous up to alignment"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_monotonic_and_aligned_within_epoch(self):
        increasing = all(
            ptr_to_int(ptr_a) < ptr_to_int(ptr_b)
            for (ptr_a, _, _, epoch_a), (ptr_b, _, _, epoch_b) in zip(self.allocations, self.allocations[1:])
            if epoch_a == epoch_b
        )
        aligned = all(
            (ptr_to_int(ptr_a) % align_a == 0)
            and align_a > 0 and (align_a & (align_a - 1)) == 0
            and MIN_ALIGNMENT <= align_a <= MAX_ALIGNMENT
            for (ptr_a, _, align_a, _) in self.allocations
        )
        positive = all(size_a > 0 for (_, size_a, _, _) in self.allocations)
        assert increasing and aligned and positive

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) > 0)
    def inv_within_capacity_span(self):
        (first_ptr, _, _, _) = self.allocations[0]
        (last_ptr, last_size, _, _) = self.allocations[-1]
        span = (ptr_to_int(last_ptr) + last_size) - ptr_to_int(first_ptr)
        assert span <= self.capacity, f"Used {span} bytes exceeds capacity {self.capacity}"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_epoch_boundary_contiguity(self):
        # After an unwind, the first allocation of the next epoch should start at the aligned end of the previous one
        assert all(
            (ptr_to_int(ptr_b) == align_up(ptr_to_int(ptr_a) + size_a, align_b))
            for (ptr_a, size_a, align_a, epoch_a), (ptr_b, size_b, align_b, epoch_b) in zip(self.allocations, self.allocations[1:])
            if epoch_a != epoch_b
        ), "First allocation of new epoch must start at aligned end of previous epoch"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_positive_sizes(self):
        # Kept for redundancy clarity; also covered by inv_monotonic_and_aligned_within_epoch
        assert all(size_a > 0 for (_, size_a, _, _) in self.allocations)

TestStackAllocatorModel = StackAllocatorModel.TestCase
