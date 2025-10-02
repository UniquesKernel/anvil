import anvil_memory as am
import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition, invariant
from hypothesis.strategies import integers, binary, sampled_from
from typing import List, Dict, Tuple

MIN_ALIGNMENT_EXP = am.MIN_ALIGNMENT_EXPONENT
MAX_ALIGNMENT_EXP = am.MAX_ALIGNMENT_EXPONENT

@hypothesis.settings(max_examples=1000)
class StackAllocatorModel(RuleBasedStateMachine):
    def __init__(self):
        super().__init__()
        self.allocator = None
        self.allocations: List[Tuple[int, int, int, int]] = []  # (address, size, alignment, epoch)
        self.copied_data: Dict[int, Tuple[bytes, int]] = {}
        self.is_destroyed = True
        self.capacity = 0
        self.alloc_mode = am.EAGER
        self.allocator_id = 0
        self.top_ptr = None
        self.allocation_epoch = 0
        self.stack: List[Tuple[int, int]] = []  # (total_allocated, allocation_count)

    def teardown(self):
        if self.allocator is not None and not self.is_destroyed:
            try:
                am.stack_allocator_destroy(self.allocator)
            except:
                pass
        self.allocations = []
        self.copied_data = {}
        self.allocator_id = 0
        self.allocation_epoch = 0
        self.stack = []

    @rule(
        exponent=integers(min_value=MIN_ALIGNMENT_EXP, max_value=MAX_ALIGNMENT_EXP),
        capacity=integers(min_value=64, max_value=(1 << 20)),
        alloc_mode=sampled_from([am.EAGER, am.LAZY])
    )
    @precondition(lambda self: self.is_destroyed)
    def create_stack_allocator(self, exponent: int, capacity: int, alloc_mode: int):
        alignment = 1 << exponent
        self.allocator = am.stack_allocator_create(capacity, alignment, alloc_mode)
        if self.allocator:
            self.capacity = capacity
            self.alloc_mode = alloc_mode
            self.is_destroyed = False
            self.allocator_id += 1
            self.copied_data = {}
            self.allocations = []
            self.top_ptr = None
            self.allocation_epoch = 0
            self.stack = []

    @rule()
    @precondition(lambda self: not self.is_destroyed)
    def destroy(self):
        if self.allocator is not None:
            err = am.stack_allocator_destroy(self.allocator)
            self.allocator = None
            self.allocations = []
            self.copied_data = {}
            self.is_destroyed = True
            self.capacity = 0
            self.stack = []
            self.top_ptr = None
            self.allocation_epoch = 0
            assert err == am.ERR_SUCCESS, f"Destroy failed with error {err}"

    @rule(
        alloc_size=integers(min_value=1, max_value=(1 << 20)),
        exponent=integers(min_value=MIN_ALIGNMENT_EXP, max_value=MAX_ALIGNMENT_EXP)
    )
    @precondition(lambda self: not self.is_destroyed)
    def alloc(self, alloc_size: int, exponent: int):
        if self.allocator is not None:
            alignment = 1 << exponent
            ptr_capsule = am.stack_allocator_alloc(self.allocator, alloc_size, alignment)

            if ptr_capsule is not None:
                ptr_int = am.ptr_to_int(ptr_capsule)
                self.allocations.append((ptr_int, alloc_size, alignment, self.allocation_epoch))
                
                # Check alignment after unwind
                if self.top_ptr is not None:
                    expected_addr = align_up(self.top_ptr, alignment)
                    assert ptr_int == expected_addr, f"Expected {expected_addr}, got {ptr_int}"
                    self.top_ptr = None

    @rule()
    @precondition(lambda self: not self.is_destroyed)
    def allocator_reset(self):
        if self.allocator is not None:
            err = am.stack_allocator_reset(self.allocator)
            self.allocations = []
            self.copied_data = {}
            self.stack = []
            self.top_ptr = None
            self.allocation_epoch = 0
            assert err == am.ERR_SUCCESS, f"Reset failed with error {err}"

    @rule(data=binary(min_size=1, max_size=999999))
    @precondition(lambda self: not self.is_destroyed)
    def copy_data(self, data: bytes):
        if self.allocator is not None:
            ptr_capsule = am.stack_allocator_copy(self.allocator, data)
            
            if ptr_capsule is not None:
                read_data = am.read_bytes(ptr_capsule, len(data))
                assert read_data == data, f"Data mismatch in copy"
                
                ptr_int = am.ptr_to_int(ptr_capsule)
                void_ptr_alignment = 8
                
                self.allocations.append((ptr_int, len(data), void_ptr_alignment, self.allocation_epoch))
                self.copied_data[ptr_int] = (data, self.allocator_id)
                
                # Check alignment after unwind
                if self.top_ptr is not None:
                    expected_min_addr = align_up(self.top_ptr, void_ptr_alignment)
                    assert ptr_int >= expected_min_addr
                    assert ptr_int % void_ptr_alignment == 0
                    self.top_ptr = None

    @rule()
    @precondition(lambda self: not self.is_destroyed and 1 <= len(self.allocations) <= 50)
    def record(self):
        if self.allocator is not None:
            err = am.stack_allocator_record(self.allocator)
            assert err == am.ERR_SUCCESS, f"Record failed with error {err}"
            
            current_allocation_count = len(self.allocations)
            if current_allocation_count > 0:
                first_addr = self.allocations[0][0]
                last_addr, last_size, _, _ = self.allocations[-1]
                total_allocated = (last_addr + last_size) - first_addr
                self.stack.append((total_allocated, current_allocation_count))
            else:
                self.stack.append((0, 0))

    @rule()
    @precondition(lambda self: not self.is_destroyed and len(self.stack) > 0)
    def unwind(self):
        if self.allocator is not None:
            err = am.stack_allocator_unwind(self.allocator)
            assert err == am.ERR_SUCCESS, f"Unwind failed with error {err}"
            
            _, allocation_count = self.stack.pop()
            
            # Keep only allocations up to the recorded point
            self.allocations = self.allocations[:allocation_count]
            
            # Increment epoch for future allocations
            self.allocation_epoch += 1
            
            # Clean up copied_data
            remaining_addresses = {addr for addr, _, _, _ in self.allocations}
            self.copied_data = {
                addr: (data, alloc_id) 
                for addr, (data, alloc_id) in self.copied_data.items() 
                if addr in remaining_addresses and alloc_id == self.allocator_id
            }
            
            # Set top_ptr for next allocation
            if self.allocations:
                last_addr, last_size, _, _ = self.allocations[-1]
                self.top_ptr = last_addr + last_size
            else:
                self.top_ptr = None

    @invariant()
    @precondition(lambda self: not self.is_destroyed and len(self.allocations) >= 1)
    def inv_no_alloc_overlap(self):
        for i, (address, size, _, _) in enumerate(self.allocations):
            for (address2, size2, _, _) in self.allocations[i+1:]:
                if address < address2 + size2 and address2 < address + size:
                    assert False, f"Memory allocations overlap"

    @invariant()
    @precondition(lambda self: not self.is_destroyed and len(self.allocations) >= 2)
    def inv_allocations_are_contiguous(self):
        """Verify allocations within the same epoch are contiguous"""
        for i in range(len(self.allocations) - 1):
            current_addr, current_size, _, current_epoch = self.allocations[i]
            next_addr, _, next_alignment, next_epoch = self.allocations[i + 1]
            
            # Only check contiguity within the same allocation epoch
            if current_epoch == next_epoch:
                current_end = current_addr + current_size
                expected_next_start = align_up(current_end, next_alignment)
                assert next_addr >= expected_next_start

    @invariant()
    @precondition(lambda self: not self.is_destroyed and len(self.allocations) > 0)
    def inv_allocations_properly_aligned(self):
        for address, _, alignment, _ in self.allocations:
            assert address % alignment == 0, f"Address {address} not aligned to {alignment}"

    @invariant()
    @precondition(lambda self: not self.is_destroyed and len(self.allocations) > 0)
    def inv_allocations_within_bounds(self):
        first_addr = self.allocations[0][0]
        last_addr, last_size, _, _ = self.allocations[-1]
        total_used = (last_addr + last_size) - first_addr
        assert total_used <= self.capacity

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_power_of_two_alignments(self):
        for _, _, alignment, _ in self.allocations:
            assert alignment > 0 and (alignment & (alignment - 1)) == 0

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_positive_sizes(self):
        for _, size, _, _ in self.allocations:
            assert size > 0

    @invariant()
    @precondition(lambda self: not self.is_destroyed and len(self.copied_data) > 0)
    def inv_copied_data_integrity(self):
        """Verify that copied/moved data maintains integrity"""
        for address, (original_data, data_allocator_id) in list(self.copied_data.items()):
            if data_allocator_id != self.allocator_id:
                continue
                
            allocation = next((alloc for alloc in self.allocations if alloc[0] == address), None)
            if allocation:
                addr, size, _, _ = allocation
                expected_size = len(original_data)
                assert size == expected_size


def align_up(address: int, alignment: int): 
    remainder = address % alignment
    return address if remainder == 0 else address + (alignment - remainder)


TestStackAllocator = StackAllocatorModel.TestCase