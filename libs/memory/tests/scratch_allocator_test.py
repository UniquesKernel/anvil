import ctypes
import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition, invariant
from hypothesis.strategies import integers
from typing import List

lib = ctypes.CDLL("libmemory_test_shared.so")

class Error:
    SUCCESS = 0
    OUT_OF_MEMORY = 1
    INVALID_ARGUMENT = 2
    INVALID_STATE = 3

# Define ctypes types
size_t = ctypes.c_size_t
void_p = ctypes.c_void_p
char_p = ctypes.c_char_p

class ScratchAllocator(ctypes.Structure):
    pass

ScratchAllocatorPtr = ctypes.POINTER(ScratchAllocator)
ScratchAllocatorPtrPtr = ctypes.POINTER(ScratchAllocatorPtr)

# Define function signatures
lib.anvil_memory_scratch_allocator_create.argtypes = [size_t, size_t]
lib.anvil_memory_scratch_allocator_create.restype = ScratchAllocatorPtr

lib.anvil_memory_scratch_allocator_destroy.argtypes = [ScratchAllocatorPtrPtr]
lib.anvil_memory_scratch_allocator_destroy.restype = ctypes.c_int

lib.anvil_memory_scratch_allocator_alloc.argtypes = [ScratchAllocatorPtr, size_t, size_t]
lib.anvil_memory_scratch_allocator_alloc.restype = void_p

lib.anvil_memory_scratch_allocator_reset.argtypes = [ScratchAllocatorPtr]
lib.anvil_memory_scratch_allocator_reset.restype = ctypes.c_int

lib.anvil_memory_scratch_allocator_copy.argtypes = [ScratchAllocatorPtr, void_p, size_t]
lib.anvil_memory_scratch_allocator_copy.restype = void_p

lib.anvil_memory_scratch_allocator_move.argtypes = [ScratchAllocatorPtr, ctypes.POINTER(void_p), size_t, ctypes.c_void_p]
lib.anvil_memory_scratch_allocator_move.restype = void_p

@hypothesis.settings(max_examples=10000)
class ScratchAllocatorModel(RuleBasedStateMachine):
    def __init__(self):
        super().__init__()
        self.allocator = None
        self.allocations: List[tuple[int,int, int]] = []
        self.is_destroyed = True
        self.capacity = 0

    def teardown(self):
        if self.allocator is not None:
            lib.anvil_memory_scratch_allocator_destroy(ctypes.pointer(self.allocator))
        self.allocations = []
   
    @rule(
            exponent=integers(min_value=3, max_value=22),
            capacity=integers(min_value=1, max_value=(1 << 20))
    )
    @precondition(lambda self: self.is_destroyed == True)
    def create_scratch_allocator(self, exponent: int, capacity: int):
        alignment = 1 << exponent

        self.allocator = lib.anvil_memory_scratch_allocator_create(capacity, alignment)
        self.capacity = capacity
        self.is_destroyed = False

    @rule()
    @precondition(lambda self: self.is_destroyed == False)
    def destroy(self):
        if self.allocator is not None:
            allocator_ptr_ptr = ctypes.pointer(self.allocator)
            lib.anvil_memory_scratch_allocator_destroy(allocator_ptr_ptr)
            self.allocator = None
            self.allocations = []
            self.is_destroyed = True
            self.capacity = 0

    @rule(
            alloc_size=integers(min_value=1, max_value=(1 << 20)),
            exponent=integers(min_value=3, max_value=22)
    )
    @precondition(lambda self: self.is_destroyed == False)
    def alloc(self, alloc_size:int, exponent: int):
        if self.allocator is not None:
            alignment = 1 << exponent
            ptr = lib.anvil_memory_scratch_allocator_alloc(self.allocator, alloc_size, alignment)

            if ptr:
                self.allocations.append((ptr, alloc_size, alignment))

    @rule()
    @precondition(lambda self: self.is_destroyed == False)
    def allocator_reset(self):
        if self.allocator is not None:
            err = lib.anvil_memory_scratch_allocator_reset(self.allocator)
            self.allocations = []
            assert err == Error.SUCCESS, f"Allocator reset failed with error code {err}"

    @invariant()
    def inv_no_alloc_overlap(self):
        if len(self.allocations) <= 1:
            return
        
        for i, (address, size,_) in enumerate(self.allocations):
            for (address2, size2,_) in self.allocations[i+1:]:
                if address < address2 + size2 and address2 < address + size:
                    assert False, f"Memory allocations overlap: allocation at {address} (size {size}) overlaps with allocation at {address2} (size {size2})"
    
    @invariant()
    def inv_allocations_are_contiguous(self):
        if len(self.allocations) <= 1:
            return

        for i, (address, size, _) in enumerate(self.allocations):
            if i == len(self.allocations) - 1:
                break

            next_address, _, next_alignment = self.allocations[i + 1]
        
            # Calculate where this allocation ends
            current_end = address + size
        
            # Calculate where the next allocation should start (aligned)
            expected_next_start = align_up(current_end, next_alignment) # type: ignore
        
            # Check if allocations are properly contiguous with alignment
            assert expected_next_start == next_address, f"Allocations not contiguous: expected {expected_next_start}, got {next_address}"

    @invariant()
    def inv_allocations_properly_aligned(self):
        for address, _, alignment in self.allocations:
            if address % alignment != 0:
                assert False, f"Address {address} not aligned to {alignment}"
        assert True

    @invariant()
    def inv_allocations_within_bounds(self):
        if not self.allocations:
            assert True
            return
        
        # Calculate total allocated space including padding
        first_addr = self.allocations[0][0]
        last_addr, last_size, _ = self.allocations[-1]
        total_used = (last_addr + last_size) - first_addr
        
        # Should not exceed the allocator's capacity
        # (You'd need to track capacity in your model)
        assert total_used <= self.capacity, f"Used {total_used} bytes exceeds capacity {self.capacity}"

    @invariant()
    def inv_power_of_two_alignments(self):
        for _, _, alignment in self.allocations:
            if alignment <= 0 or (alignment & (alignment - 1)) != 0:
                assert False, f"Alignment {alignment} is not a power of 2"
        assert True

    @invariant()
    def inv_positive_sizes(self):
        for _, size, _ in self.allocations:
            if size <= 0:
                assert False, f"Invalid allocation size: {size}"
        assert True

def align_up(address: int, alignment: int): 
    """Align address up to next alignment boundary"""
    remainder = address % alignment
    return address if remainder == 0 else address + (alignment - remainder)


TestMyStateMachine = ScratchAllocatorModel.TestCase # type: ignore