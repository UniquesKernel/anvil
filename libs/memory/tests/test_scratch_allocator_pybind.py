import anvil_memory as am
import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition, invariant
from hypothesis.strategies import integers, binary
from typing import List, Dict

# IMPORTANT: Use exponent ranges, not alignment values!
MIN_ALIGNMENT_EXP = am.MIN_ALIGNMENT_EXPONENT  # 0
MAX_ALIGNMENT_EXP = am.MAX_ALIGNMENT_EXPONENT  # 11

@hypothesis.settings(max_examples=1000)
class ScratchAllocatorModel(RuleBasedStateMachine):
    def __init__(self):
        super().__init__()
        self.allocator = None
        self.allocations: List[tuple[int, int, int]] = []
        self.copied_data: Dict[int, tuple[bytes, int]] = {}
        self.is_destroyed = True
        self.capacity = 0
        self.allocator_id = 0

    def teardown(self):
        if self.allocator is not None and not self.is_destroyed:
            try:
                am.scratch_allocator_destroy(self.allocator)
            except:
                pass
        self.allocations = []
        self.copied_data = {}
        self.allocator_id = 0

    @rule(
        exponent=integers(min_value=MIN_ALIGNMENT_EXP, max_value=MAX_ALIGNMENT_EXP),  # FIXED
        capacity=integers(min_value=64, max_value=(1 << 20))
    )
    @precondition(lambda self: self.is_destroyed)
    def create_scratch_allocator(self, exponent: int, capacity: int):
        alignment = 1 << exponent
        self.allocator = am.scratch_allocator_create(capacity, alignment)
        if self.allocator:
            self.capacity = capacity
            self.is_destroyed = False
            self.allocator_id += 1
            self.copied_data = {}
            self.allocations = []

    @rule()
    @precondition(lambda self: not self.is_destroyed)
    def destroy(self):
        if self.allocator is not None:
            err = am.scratch_allocator_destroy(self.allocator)
            self.allocator = None
            self.allocations = []
            self.copied_data = {}
            self.is_destroyed = True
            self.capacity = 0
            assert err == am.ERR_SUCCESS, f"Destroy failed with error {err}"

    @rule(
        alloc_size=integers(min_value=1, max_value=(1 << 20)),
        exponent=integers(min_value=MIN_ALIGNMENT_EXP, max_value=MAX_ALIGNMENT_EXP)  # FIXED
    )
    @precondition(lambda self: not self.is_destroyed)
    def alloc(self, alloc_size: int, exponent: int):
        if self.allocator is not None:
            alignment = 1 << exponent
            ptr_capsule = am.scratch_allocator_alloc(self.allocator, alloc_size, alignment)
            
            if ptr_capsule is not None:
                ptr_int = am.ptr_to_int(ptr_capsule)
                self.allocations.append((ptr_int, alloc_size, alignment))

    @rule()
    @precondition(lambda self: not self.is_destroyed)
    def allocator_reset(self):
        if self.allocator is not None:
            err = am.scratch_allocator_reset(self.allocator)
            self.allocations = []
            self.copied_data = {}
            assert err == am.ERR_SUCCESS, f"Reset failed with error {err}"

    @rule(data=binary(min_size=1, max_size=999999))
    @precondition(lambda self: not self.is_destroyed)
    def copy_data(self, data: bytes):
        if self.allocator is not None:
            ptr_capsule = am.scratch_allocator_copy(self.allocator, data)
            
            if ptr_capsule is not None:
                read_data = am.read_bytes(ptr_capsule, len(data))
                assert read_data == data, f"Data mismatch"
                
                ptr_int = am.ptr_to_int(ptr_capsule)
                void_ptr_alignment = 8
                
                self.allocations.append((ptr_int, len(data), void_ptr_alignment))
                self.copied_data[ptr_int] = (data, self.allocator_id)

    @invariant()
    @precondition(lambda self: not self.is_destroyed and len(self.allocations) >= 1)
    def inv_no_alloc_overlap(self):
        for i, (address, size, _) in enumerate(self.allocations):
            for (address2, size2, _) in self.allocations[i+1:]:
                if address < address2 + size2 and address2 < address + size:
                    assert False, f"Memory allocations overlap"

    @invariant()
    @precondition(lambda self: not self.is_destroyed and len(self.allocations) >= 2)
    def inv_allocations_are_contiguous(self):
        for i in range(len(self.allocations) - 1):
            address, size, _ = self.allocations[i]
            next_address, _, next_alignment = self.allocations[i + 1]
            current_end = address + size
            expected_next_start = align_up(current_end, next_alignment)
            assert expected_next_start == next_address

    @invariant()
    @precondition(lambda self: not self.is_destroyed and len(self.allocations) > 0)
    def inv_allocations_properly_aligned(self):
        for address, _, alignment in self.allocations:
            assert address % alignment == 0, f"Address {address} not aligned to {alignment}"

    @invariant()
    @precondition(lambda self: not self.is_destroyed and len(self.allocations) > 0)
    def inv_allocations_within_bounds(self):
        first_addr = self.allocations[0][0]
        last_addr, last_size, _ = self.allocations[-1]
        total_used = (last_addr + last_size) - first_addr
        assert total_used <= self.capacity

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_power_of_two_alignments(self):
        for _, _, alignment in self.allocations:
            assert alignment > 0 and (alignment & (alignment - 1)) == 0

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_positive_sizes(self):
        for _, size, _ in self.allocations:
            assert size > 0


def align_up(address: int, alignment: int): 
    remainder = address % alignment
    return address if remainder == 0 else address + (alignment - remainder)


TestScratchAllocator = ScratchAllocatorModel.TestCase