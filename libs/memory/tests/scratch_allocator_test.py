import ctypes
import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition, invariant
from hypothesis.strategies import integers, binary
from typing import List, Dict

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

# Standard library functions for memory operations
libc = ctypes.CDLL("libc.so.6")
libc.malloc.argtypes = [size_t]
libc.malloc.restype = void_p
libc.free.argtypes = [void_p]
libc.free.restype = None
libc.memcmp.argtypes = [void_p, void_p, size_t]
libc.memcmp.restype = ctypes.c_int

MIN_ALIGNMENT = 1
MAX_ALIGNMENT = 22

@hypothesis.settings(max_examples=10000)
class ScratchAllocatorModel(RuleBasedStateMachine):
    def __init__(self):
        super().__init__()
        self.allocator = None
        self.allocations: List[tuple[int, int, int]] = []  # (address, size, alignment)
        self.copied_data: Dict[int, tuple[bytes, int]] = {}  # address -> (original data, allocator_id)
        self.is_destroyed = True
        self.capacity = 0
        self.external_allocations: List[int] = []  # Track malloc'd pointers for cleanup
        self.allocator_id = 0  # Track allocator lifecycle

    def teardown(self):
        if self.allocator is not None:
            lib.anvil_memory_scratch_allocator_destroy(ctypes.pointer(self.allocator))
        
        for ptr in self.external_allocations:
            if ptr:
                libc.free(ptr)
        self.allocations = []
        self.copied_data = {}
        self.external_allocations = []
        self.allocator_id = 0

    def _cleanup_stale_copied_data(self):
        """Remove copied_data entries that don't belong to current allocator lifecycle"""
        self.copied_data = {
            addr: (data, alloc_id) 
            for addr, (data, alloc_id) in self.copied_data.items() 
            if alloc_id == self.allocator_id
        }
   
    @rule(
            exponent=integers(min_value=MIN_ALIGNMENT, max_value=MAX_ALIGNMENT),
            capacity=integers(min_value=64, max_value=(1 << 20))  # Increased minimum capacity
    )
    @precondition(lambda self: self.is_destroyed == True)
    def create_scratch_allocator(self, exponent: int, capacity: int):
        alignment = 1 << exponent

        self.allocator = lib.anvil_memory_scratch_allocator_create(capacity, alignment)
        self.capacity = capacity
        self.is_destroyed = False
        self.allocator_id += 1
        self.copied_data = {}
        self.allocations = []

    @rule()
    @precondition(lambda self: self.is_destroyed == False)
    def destroy(self):
        if self.allocator is not None:
            allocator_ptr_ptr = ctypes.pointer(self.allocator)
            err = lib.anvil_memory_scratch_allocator_destroy(allocator_ptr_ptr)
            self.allocator = None
            self.allocations = []
            self.copied_data = {}
            self.is_destroyed = True
            self.capacity = 0
            assert err == Error.SUCCESS, f"Allocator destruction failed with error code {err}"

    @rule(
            alloc_size=integers(min_value=1, max_value=(1 << 20)),
            exponent=integers(min_value=MIN_ALIGNMENT, max_value=MAX_ALIGNMENT)
    )
    @precondition(lambda self: self.is_destroyed == False)
    def alloc(self, alloc_size: int, exponent: int):
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
            self.copied_data = {}
            assert err == Error.SUCCESS, f"Allocator reset failed with error code {err}"

    @rule(data=binary(min_size=1, max_size=999999))
    @precondition(lambda self: self.is_destroyed == False) 
    def copy_data(self, data: bytes):
        if self.allocator is not None:
            # Create source buffer with the test data
            src_size = len(data)
            src_buffer = (ctypes.c_char * src_size).from_buffer_copy(data)
            src_ptr = ctypes.cast(src_buffer, void_p)
            
            # Call the copy function
            dest_ptr = lib.anvil_memory_scratch_allocator_copy(
                self.allocator, src_ptr, src_size
            )
            
            if dest_ptr:
                # Verify the data was copied correctly immediately
                dest_buffer = (ctypes.c_char * src_size).from_address(dest_ptr)
                copied_data = bytes(dest_buffer)
                assert copied_data == data, f"Data mismatch in copy: expected {data}, got {copied_data}"
                
                # Track this allocation (copy uses alignof(void*) alignment)
                void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
                
                # Defensive check: ensure we're not tracking duplicate addresses
                existing_alloc = next((alloc for alloc in self.allocations if alloc[0] == dest_ptr), None)
                assert existing_alloc is None, f"Address {dest_ptr} already tracked in allocations"
                
                self.allocations.append((dest_ptr, src_size, void_ptr_alignment))
                self.copied_data[dest_ptr] = (data, self.allocator_id)

    @rule(data=binary(min_size=1, max_size=999999))   
    @precondition(lambda self: self.is_destroyed == False)
    def move_data(self, data: bytes):
        if self.allocator is not None:
            # Allocate source buffer using malloc
            src_size = len(data)
            src_ptr = libc.malloc(src_size)
            
            if not src_ptr:
                return  # malloc failed, skip this test
            
            # Track the malloc'd pointer for cleanup
            self.external_allocations.append(src_ptr)
            
            # Copy test data to the malloc'd buffer
            src_buffer = (ctypes.c_char * src_size).from_address(src_ptr)
            for i, byte in enumerate(data):
                src_buffer[i] = byte
            
            # Create a pointer to the source pointer for the move function
            src_ptr_ref = ctypes.pointer(ctypes.c_void_p(src_ptr))
            
            # Get the free function pointer
            free_func = ctypes.cast(libc.free, ctypes.c_void_p)
            
            # Call the move function
            dest_ptr = lib.anvil_memory_scratch_allocator_move(
                self.allocator, src_ptr_ref, src_size, free_func
            )
            
            if dest_ptr:
                # Verify the data was moved correctly
                dest_buffer = (ctypes.c_char * src_size).from_address(dest_ptr)
                moved_data = bytes(dest_buffer)
                assert moved_data == data, f"Data mismatch in move: expected {data}, got {moved_data}"
                
                # Verify the source pointer was set to NULL
                assert src_ptr_ref.contents.value is None or src_ptr_ref.contents.value == 0, \
                    "Source pointer should be NULL after move"
                
                # Remove from external allocations since it was freed by move
                if src_ptr in self.external_allocations:
                    self.external_allocations.remove(src_ptr)
                
                # Track this allocation (move uses alignof(void*) alignment)
                void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
                
                # Defensive check: ensure we're not tracking duplicate addresses
                existing_alloc = next((alloc for alloc in self.allocations if alloc[0] == dest_ptr), None)
                assert existing_alloc is None, f"Address {dest_ptr} already tracked in allocations"
                
                self.allocations.append((dest_ptr, src_size, void_ptr_alignment))
                self.copied_data[dest_ptr] = (data, self.allocator_id)

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) >= 1)
    def inv_no_alloc_overlap(self):
        for i, (address, size, _) in enumerate(self.allocations):
            for (address2, size2, _) in self.allocations[i+1:]:
                if address < address2 + size2 and address2 < address + size:
                    assert False, f"Memory allocations overlap: allocation at {address} (size {size}) overlaps with allocation at {address2} (size {size2})"
    
    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) >= 1)
    def inv_allocations_are_contiguous(self):
        for i, (address, size, _) in enumerate(self.allocations):
            if i == len(self.allocations) - 1:
                break

            next_address, _, next_alignment = self.allocations[i + 1]
        
            # Calculate where this allocation ends
            current_end = address + size
        
            # Calculate where the next allocation should start (aligned)
            expected_next_start = align_up(current_end, next_alignment)
        
            # Check if allocations are properly contiguous with alignment
            assert expected_next_start == next_address, f"Allocations not contiguous: expected {expected_next_start}, got {next_address}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False)
    def inv_allocations_properly_aligned(self):
        for address, _, alignment in self.allocations:
            if address % alignment != 0:
                assert False, f"Address {address} not aligned to {alignment}"

    @invariant()
    def inv_allocations_within_bounds(self):
        if not self.allocations or self.is_destroyed:
            return
        
        # Calculate total allocated space including padding
        first_addr = self.allocations[0][0]
        last_addr, last_size, _ = self.allocations[-1]
        total_used = (last_addr + last_size) - first_addr
        
        # Should not exceed the allocator's capacity
        assert total_used <= self.capacity, f"Used {total_used} bytes exceeds capacity {self.capacity}"

    @invariant()
    def inv_power_of_two_alignments(self):
        for _, _, alignment in self.allocations:
            if alignment <= 0 or (alignment & (alignment - 1)) != 0:
                assert False, f"Alignment {alignment} is not a power of 2"

    @invariant()
    def inv_positive_sizes(self):
        for _, size, _ in self.allocations:
            if size <= 0:
                assert False, f"Invalid allocation size: {size}"

    @invariant()
    def inv_copied_data_integrity(self):
        """Verify that copied/moved data maintains integrity"""
        # Skip if allocator is destroyed
        if self.is_destroyed:
            return
            
        # Clean up any stale entries from previous allocator lifecycles
        self._cleanup_stale_copied_data()
        
        for address, (original_data, data_allocator_id) in self.copied_data.items():
            # Double-check: only validate data from current allocator lifecycle
            if data_allocator_id != self.allocator_id:
                continue
                
            # Find the allocation that corresponds to this address
            allocation = next((alloc for alloc in self.allocations if alloc[0] == address), None)
            if allocation:
                addr, size, _ = allocation
                
                # The allocation size should match the original data size for copy/move operations
                expected_size = len(original_data)
                assert size == expected_size, \
                    f"Size mismatch at {addr}: allocation size {size} != original data size {expected_size}"
                
                # Verify the data is still intact
                buffer = (ctypes.c_char * expected_size).from_address(addr)
                current_data = bytes(buffer)
                assert current_data == original_data, \
                    f"Data corruption detected at {addr}: expected {original_data}, got {current_data}"

def align_up(address: int, alignment: int): 
    """Align address up to next alignment boundary"""
    remainder = address % alignment
    return address if remainder == 0 else address + (alignment - remainder)

TestMyStateMachine = ScratchAllocatorModel.TestCase # type: ignore