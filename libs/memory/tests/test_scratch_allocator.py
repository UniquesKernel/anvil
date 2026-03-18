from anvil_memory.scratch_allocator import scratch_allocator_create, scratch_allocator_destroy, scratch_allocator_reset, scratch_allocator_alloc, scratch_allocator_write, scratch_allocator_read
from anvil_memory import Error, MAX_ALIGNMENT, MIN_ALIGNMENT, ptr_to_int
from hypothesis.stateful import RuleBasedStateMachine, initialize, precondition, rule, invariant
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

class ScratchAllocatorModel(RuleBasedStateMachine):
    
    def __init__(self) -> None:
        super().__init__() 
        self.allocator = None
        self.isValid = False
        self.capacity = 0
        self.allocated = 0
        self.alignment = 0
        self.allocations = []
    
    @initialize(config=allocator_config())
    def initialize_allocator(self, config):
        capacity, alignment = config
        err, allocator = scratch_allocator_create(capacity,alignment)

        if err != Error.OK or allocator is None:
            self.isValid = False
            return

        assert err == Error.OK
        assert allocator is not None

        self.allocator = allocator
        self.isValid = True
        self.capacity = capacity
        self.alignment = alignment

    @rule(config=allocator_config())
    @precondition(lambda self: self.isValid == False)
    def create(self, config):
        capacity, alignment = config
        err, allocator = scratch_allocator_create(capacity, alignment)

        if err != Error.OK or allocator is None:
            self.isValid = False
            return

        assert err == Error.OK
        assert allocator is not None

        self.allocator = allocator
        self.isValid = True
        self.capacity = capacity
        self.alignment = alignment

    @rule(config=allocator_config())
    @precondition(lambda self: self.allocator is not None)
    def alloc(self, config):
        allocation_size, alignment = config
        ptr, _ = scratch_allocator_alloc(self.allocator, allocation_size, alignment)

        if (self.isValid == False):
            assert ptr is None
            return

        if ptr:
            self.allocations.append((ptr, allocation_size, alignment))
            self.allocated = self.allocated + allocation_size

    @rule(data=st.data())
    @precondition(lambda self: self.isValid and len(self.allocations) > 0)
    def write_to_allocation(self, data):
        idx = data.draw(st.integers(min_value=0, max_value=len(self.allocations) - 1))
        ptr, allocation_size, _ = self.allocations[idx]

        payload = bytes(i & 0xFF for i in range(allocation_size))
        err = scratch_allocator_write(ptr, payload)
        assert err == Error.OK

        err, result = scratch_allocator_read(ptr, allocation_size)
        assert err == Error.OK
        assert result == payload, "Write/read mismatch"

    @rule()
    @precondition(lambda self: self.allocator is not None)
    def allocator_reset(self):
        err = scratch_allocator_reset(self.allocator)

        if (self.isValid == True):
            assert err == Error.OK, f"Allocator reset failed with error code {err}"
            self.allocated = 0
            self.allocations.clear()
        else:
            assert err == Error.NULL_PARAMETER

    @rule()
    @precondition(lambda self: self.allocator is not None)
    def destroy(self): 
        err = scratch_allocator_destroy(self.allocator)
        if (self.isValid == True):
            assert err == Error.OK
            self.allocations.clear()
            self.isValid = False
            self.allocated = 0
            self.capacity = 0
            self.alignment = 0
        else:
            assert err == Error.NULL_PARAMETER

    @invariant()
    def inv_contiguous_adjacent(self):
        if len(self.allocations) < 2:
            return  # Need at least 2 allocations to check adjacency
        
        for (ptr_a, size_a, _), (ptr_b, _, align_b) in pairwise(self.allocations):
            addr_a = ptr_to_int(ptr_a)
            addr_b = ptr_to_int(ptr_b)
            
            # Second allocation should start at aligned address after first
            expected_b = align_up(addr_a + size_a, align_b)
            assert addr_b == expected_b, \
                f"Allocations not contiguous: addr={addr_a} size={size_a} " \
                f"aligned to {align_b} = {expected_b}, but next starts at {addr_b}"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 1)
    def inv_monotonic(self):
        for (ptr_a, _, _), (ptr_b, _, _) in pairwise(self.allocations):
            assert (ptr_to_int(ptr_a) < ptr_to_int(ptr_b))

    @invariant()
    def inv_aligned(self):
        for (ptr_a, _, align_a) in self.allocations:
            assert ptr_to_int(ptr_a) % align_a == 0
            assert align_a > 0
            assert (align_a & (align_a - 1)) == 0
            assert MIN_ALIGNMENT <= align_a <= MAX_ALIGNMENT 

    @invariant()
    def inv_positive_size(self):
        for (_, size_a, _) in self.allocations:
            assert size_a > 0

    @invariant()
    @precondition(lambda self: len(self.allocations) > 1)
    def inv_no_overlaps(self):
        addresses = [(ptr_to_int(ptr_a), ptr_to_int(ptr_a) + size_a) for (ptr_a, size_a, _) in self.allocations]
        intervals = sorted (addresses)

        for prev, curr in zip(intervals, intervals[1:]):
            assert prev[1] <= curr[0]

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) >= 1)
    def inv_within_capacity_span(self):
        first_ptr, _, _ = self.allocations[0]
        first_start = ptr_to_int(first_ptr)

        last_ptr, last_size, _ = self.allocations[-1]
        last_addr = ptr_to_int(last_ptr)
        last_end = last_addr + last_size
        assert (last_end - first_start) <= self.capacity, (
            f"Used {(last_end - first_start)} > capacity {self.capacity}"
        )

TestScratchAllocatorModel = ScratchAllocatorModel.TestCase
