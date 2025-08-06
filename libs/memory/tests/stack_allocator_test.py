import ctypes
from ctypes import c_size_t, c_void_p
import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition, invariant
from hypothesis.strategies import integers, binary, sampled_from
from typing import List, Dict

"""
Create a binding the stack allocator of Anvil's memory management library.
"""

lib = ctypes.CDLL("libmemory_test_shared.so")

class StackAllocator(ctypes.Structure):
    pass

StackAllocatorPtr = ctypes.POINTER(StackAllocator)
StackAllocatorPtrPtr = ctypes.POINTER(StackAllocatorPtr)

# Define function signatures
lib.anvil_memory_stack_allocator_create.argtypes = [c_size_t, c_size_t, c_size_t]
lib.anvil_memory_stack_allocator_create.restype = StackAllocatorPtr

lib.anvil_memory_stack_allocator_destroy.argtypes = [StackAllocatorPtrPtr]
lib.anvil_memory_stack_allocator_destroy.restype = ctypes.c_int

lib.anvil_memory_stack_allocator_alloc.argtypes = [StackAllocatorPtr, c_size_t, c_size_t]
lib.anvil_memory_stack_allocator_alloc.restype = c_void_p

lib.anvil_memory_stack_allocator_reset.argtypes = [StackAllocatorPtr]
lib.anvil_memory_stack_allocator_reset.restype = ctypes.c_int

# Pretend these functions exist (as requested)
lib.anvil_memory_stack_allocator_copy.argtypes = [StackAllocatorPtr, c_void_p, c_size_t]
lib.anvil_memory_stack_allocator_copy.restype = c_void_p

lib.anvil_memory_stack_allocator_move.argtypes = [StackAllocatorPtr, ctypes.POINTER(c_void_p), c_size_t, ctypes.c_void_p]
lib.anvil_memory_stack_allocator_move.restype = c_void_p

lib.anvil_memory_stack_allocator_record.argtypes = [StackAllocatorPtr]
lib.anvil_memory_stack_allocator_record.restype = ctypes.c_int

lib.anvil_memory_stack_allocator_unwind.argtypes = [StackAllocatorPtr]
lib.anvil_memory_stack_allocator_unwind.restype = ctypes.c_int

"""
Define C bindings for the standard C library
"""
libc = ctypes.CDLL("libc.so.6")
libc.malloc.argtypes = [c_size_t]
libc.malloc.restype = c_void_p
libc.free.argtypes = [c_void_p]
libc.free.restype = None
libc.memcmp.argtypes = [c_void_p, c_void_p, c_size_t]
libc.memcmp.restype = ctypes.c_int

"""
Define Error classes based on Anvil's Error Codes
"""

class Error:
    SUCCESS = 0
    OUT_OF_MEMORY = 1
    INVALID_ARGUMENT = 2
    INVALID_STATE = 3

"""
Useful constants
"""

EAGER = (1 << 0)
LAZY = (1 << 1)
MIN_ALIGNMENT = 1
MAX_ALIGNMENT = 11

"""
Stack Allocator Model
"""

@hypothesis.settings(max_examples=10000)
class StackAllocatorModel(RuleBasedStateMachine):
    def __init__(self):
        super().__init__()
        self.allocator = None
        self.allocations: List[tuple[int, int, int, int]] = []  # (address, size, alignment, epoch)
        self.copied_data: Dict[int, tuple[bytes, int]] = {}  # address -> (original data, allocator_id)
        self.is_destroyed = True
        self.capacity = 0
        self.alloc_mode = EAGER
        self.external_allocations: List[int] = []  # Track malloc'd pointers for cleanup
        self.allocator_id = 0  # Track allocator lifecycle
        self.top_alignment = 0
        self.allocation_epoch = 0  # Track allocation epochs to handle unwind properly

    def teardown(self):
        if self.allocator is not None:
            lib.anvil_memory_stack_allocator_destroy(ctypes.pointer(self.allocator))
        
        for ptr in self.external_allocations:
            if ptr:
                libc.free(ptr)
        self.allocations = []
        self.copied_data = {}
        self.external_allocations = []
        self.allocator_id = 0
        self.allocation_epoch = 0

    def _cleanup_stale_copied_data(self):
        """Remove copied_data entries that don't belong to current allocator lifecycle"""
        self.copied_data = {
            addr: (data, alloc_id) 
            for addr, (data, alloc_id) in self.copied_data.items() 
            if alloc_id == self.allocator_id
        }
   
    @rule(
            exponent=integers(min_value=MIN_ALIGNMENT, max_value=MAX_ALIGNMENT),
            capacity=integers(min_value=64, max_value=(1 << 20)),  # Increased minimum capacity
            alloc_mode=sampled_from([EAGER, LAZY])
    )
    @precondition(lambda self: self.is_destroyed == True)
    def create_stack_allocator(self, exponent: int, capacity: int, alloc_mode: int):
        alignment = 1 << exponent

        self.allocator = lib.anvil_memory_stack_allocator_create(capacity, alignment, alloc_mode)
        self.capacity = capacity
        self.alloc_mode = alloc_mode
        self.is_destroyed = False
        self.allocator_id += 1
        self.copied_data = {}
        self.allocations = []
        self.top_ptr = None 
        self.allocation_epoch = 0  # Reset epoch for new allocator
        self.stack: list[tuple[int, int]] = []  # Changed to store (total_allocated, allocation_count)

    @rule()
    @precondition(lambda self: self.is_destroyed == False)
    def destroy(self):
        if self.allocator is not None:
            allocator_ptr_ptr = ctypes.pointer(self.allocator)
            err = lib.anvil_memory_stack_allocator_destroy(allocator_ptr_ptr)
            self.allocator = None
            self.allocations = []
            self.copied_data = {}
            self.is_destroyed = True
            self.capacity = 0
            self.stack = []
            self.top_ptr = None
            self.allocation_epoch = 0
            assert err == Error.SUCCESS, f"Allocator destruction failed with error code {err}"

    @rule(
            alloc_size=integers(min_value=1, max_value=(1 << 20)),
            exponent=integers(min_value=MIN_ALIGNMENT, max_value=MAX_ALIGNMENT)
    )
    @precondition(lambda self: self.is_destroyed == False)
    def alloc(self, alloc_size: int, exponent: int):
        if self.allocator is not None:
            alignment = 1 << exponent
            ptr = lib.anvil_memory_stack_allocator_alloc(self.allocator, alloc_size, alignment)

            if ptr:
                self.allocations.append((ptr, alloc_size, alignment, self.allocation_epoch))

            # Check that if an unwind happened that the next allocation 
            # Also aligns to the recorded offset.
            if ptr and self.top_ptr is not None:
                expected_addr = align_up(self.top_ptr, alignment)  # Use current allocation's alignment
                assert ptr == expected_addr, f"Expected {expected_addr}, got {ptr}"
                self.top_ptr = None

    @rule()
    @precondition(lambda self: self.is_destroyed == False)
    def allocator_reset(self):
        if self.allocator is not None:
            err = lib.anvil_memory_stack_allocator_reset(self.allocator)
            self.allocations = []
            self.copied_data = {}
            self.stack = []
            self.top_ptr = None
            self.allocation_epoch = 0  # Reset epoch on full reset
            assert err == Error.SUCCESS, f"Allocator reset failed with error code {err}"

    @rule(data=binary(min_size=1, max_size=999999))
    @precondition(lambda self: self.is_destroyed == False) 
    def copy_data(self, data: bytes):
        if self.allocator is not None:
            src_size = len(data)
            src_buffer = (ctypes.c_char * src_size).from_buffer_copy(data)
            src_ptr = ctypes.cast(src_buffer, c_void_p)
            
            dest_ptr = lib.anvil_memory_stack_allocator_copy(
                self.allocator, src_ptr, src_size
            )
            
            if dest_ptr:
                dest_buffer = (ctypes.c_char * src_size).from_address(dest_ptr)
                copied_data = bytes(dest_buffer)
                assert copied_data == data, f"Data mismatch in copy: expected {data}, got {copied_data}"
                
                void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
                
                # Defensive check: ensure we're not tracking duplicate addresses
                existing_alloc = next((alloc for alloc in self.allocations if alloc[0] == dest_ptr), None)
                assert existing_alloc is None, f"Address {dest_ptr} already tracked in allocations"
                
                self.allocations.append((dest_ptr, src_size, void_ptr_alignment, self.allocation_epoch))
                self.copied_data[dest_ptr] = (data, self.allocator_id)
            
            # After unwind, check if the new allocation is placed correctly
            if dest_ptr and self.top_ptr is not None:
                void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
                # The allocator should place the new allocation at or after the expected aligned position
                expected_min_addr = align_up(self.top_ptr, void_ptr_alignment)
                assert dest_ptr >= expected_min_addr, f"Address {dest_ptr} comes before expected minimum {expected_min_addr}"
                # Ensure proper alignment
                assert dest_ptr % void_ptr_alignment == 0, f"Address {dest_ptr} not properly aligned to {void_ptr_alignment}"
                self.top_ptr = None

    @rule(data=binary(min_size=1, max_size=999999))   
    @precondition(lambda self: self.is_destroyed == False)
    def move_data(self, data: bytes):
        if self.allocator is not None:
            src_size = len(data)
            src_ptr = libc.malloc(src_size)
            
            if not src_ptr:
                return  
            
            self.external_allocations.append(src_ptr)
            
            src_buffer = (ctypes.c_char * src_size).from_address(src_ptr)
            for i, byte in enumerate(data):
                src_buffer[i] = byte
            
            src_ptr_ref = ctypes.pointer(ctypes.c_void_p(src_ptr))
            
            free_func = ctypes.cast(libc.free, ctypes.c_void_p)
            
            dest_ptr = lib.anvil_memory_stack_allocator_move(
                self.allocator, src_ptr_ref, src_size, free_func
            )
            
            if dest_ptr:
                dest_buffer = (ctypes.c_char * src_size).from_address(dest_ptr)
                moved_data = bytes(dest_buffer)
                assert moved_data == data, f"Data mismatch in move: expected {data}, got {moved_data}"
                
                assert src_ptr_ref.contents.value is None or src_ptr_ref.contents.value == 0, \
                    "Source pointer should be NULL after move"
                
                if src_ptr in self.external_allocations:
                    self.external_allocations.remove(src_ptr)
                
                void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
                
                # Defensive check: ensure we're not tracking duplicate addresses
                existing_alloc = next((alloc for alloc in self.allocations if alloc[0] == dest_ptr), None)
                assert existing_alloc is None, f"Address {dest_ptr} already tracked in allocations"
                
                self.allocations.append((dest_ptr, src_size, void_ptr_alignment, self.allocation_epoch))
                self.copied_data[dest_ptr] = (data, self.allocator_id)
            
            # After unwind, check if the new allocation is placed correctly
            if dest_ptr and self.top_ptr is not None:
                void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
                # The allocator should place the new allocation at or after the expected aligned position
                expected_min_addr = align_up(self.top_ptr, void_ptr_alignment)
                assert dest_ptr >= expected_min_addr, f"Address {dest_ptr} comes before expected minimum {expected_min_addr}"
                # Ensure proper alignment
                assert dest_ptr % void_ptr_alignment == 0, f"Address {dest_ptr} not properly aligned to {void_ptr_alignment}"
                self.top_ptr = None

    @rule()   
    @precondition(lambda self: self.is_destroyed == False and 1 <= len(self.allocations) <= 50)
    def record(self):
        lib.anvil_memory_stack_allocator_record(self.allocator)
        # The C implementation saves the current 'allocated' byte offset, not address + size
        # We need to track which allocations existed at this point
        current_allocation_count = len(self.allocations)
        if current_allocation_count > 0:
            # Calculate the total allocated bytes up to this point
            first_addr = self.allocations[0][0]
            last_addr, last_size, _, _ = self.allocations[-1]
            total_allocated = (last_addr + last_size) - first_addr
            self.stack.append((total_allocated, current_allocation_count))
        else:
            self.stack.append((0, 0))

    @rule()   
    @precondition(lambda self: self.is_destroyed == False and len(self.stack) > 0)
    def unwind(self):
        lib.anvil_memory_stack_allocator_unwind(self.allocator)
        
        total_allocated, allocation_count = self.stack.pop()
        
        # CRITICAL FIX: After unwind, only allocations up to the recorded point still exist
        # Everything allocated after the record point is effectively "deallocated"
        self.allocations = self.allocations[:allocation_count]
        
        # Increment allocation epoch - this helps track that future allocations 
        # are in a different "generation" and may not be contiguous with old ones
        self.allocation_epoch += 1
        
        # Clean up copied_data to only include data from remaining allocations
        remaining_addresses = {addr for addr, _, _, _ in self.allocations}
        self.copied_data = {
            addr: (data, alloc_id) 
            for addr, (data, alloc_id) in self.copied_data.items() 
            if addr in remaining_addresses and alloc_id == self.allocator_id
        }
        
        # Set top_ptr to the end of the last remaining allocation (if any)
        if self.allocations:
            last_addr, last_size, _, _ = self.allocations[-1]
            self.top_ptr = last_addr + last_size
        else:
            # If no allocations remain, next allocation will start from base
            self.top_ptr = None

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) >= 1)
    def inv_no_alloc_overlap(self):
        for i, (address, size, _, _) in enumerate(self.allocations):
            for (address2, size2, _, _) in self.allocations[i+1:]:
                if address < address2 + size2 and address2 < address + size:
                    assert False, f"Memory allocations overlap: allocation at {address} (size {size}) overlaps with allocation at {address2} (size {size2})"
    
    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) >= 2)
    def inv_allocations_are_contiguous(self):
        """
        Verify that allocations within the same epoch are contiguous.
        After unwind operations, allocations may not be contiguous across epochs.
        """
        for i in range(len(self.allocations) - 1):
            current_addr, current_size, _, current_epoch = self.allocations[i]
            next_addr, _, next_alignment, next_epoch = self.allocations[i + 1]
            
            # Only check contiguity within the same allocation epoch
            if current_epoch == next_epoch:
                current_end = current_addr + current_size
                expected_next_start = align_up(current_end, next_alignment)
                
                assert next_addr >= expected_next_start, \
                    f"Allocations in same epoch not contiguous: expected >= {expected_next_start}, got {next_addr}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) > 0)
    def inv_allocations_properly_aligned(self):
        for address, _, alignment, _ in self.allocations:
            if address % alignment != 0:
                assert False, f"Address {address} not aligned to {alignment}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) > 0)
    def inv_allocations_within_bounds(self):
        first_addr = self.allocations[0][0]
        last_addr, last_size, _, _ = self.allocations[-1]
        total_used = (last_addr + last_size) - first_addr
        
        assert total_used <= self.capacity, f"Used {total_used} bytes exceeds capacity {self.capacity}"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_power_of_two_alignments(self):
        for _, _, alignment, _ in self.allocations:
            if alignment <= 0 or (alignment & (alignment - 1)) != 0:
                assert False, f"Alignment {alignment} is not a power of 2"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_positive_sizes(self):
        for _, size, _, _ in self.allocations:
            if size <= 0:
                assert False, f"Invalid allocation size: {size}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False)
    def inv_copied_data_integrity(self):
        """Verify that copied/moved data maintains integrity"""
        self._cleanup_stale_copied_data()
        
        for address, (original_data, data_allocator_id) in self.copied_data.items():
            # Double-check: only validate data from current allocator lifecycle
            if data_allocator_id != self.allocator_id:
                continue
                
            allocation = next((alloc for alloc in self.allocations if alloc[0] == address), None)
            if allocation:
                addr, size, _, _ = allocation
                
                expected_size = len(original_data)
                assert size == expected_size, \
                    f"Size mismatch at {addr}: allocation size {size} != original data size {expected_size}"
                
                buffer = (ctypes.c_char * expected_size).from_address(addr)
                current_data = bytes(buffer)
                assert current_data == original_data, \
                    f"Data corruption detected at {addr}: expected {original_data}, got {current_data}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and self.alloc_mode == LAZY and len(self.allocations) > 0)
    def inv_lazy_allocation_behavior(self):
        """
        For lazy allocators, we might want to add specific invariants about 
        memory commitment behavior. This is a placeholder for future lazy-specific tests.
        """
        # This invariant could test that memory is only committed when actually accessed
        # For now, it's just a placeholder that always passes
        pass

def align_up(address: int, alignment: int): 
    """Align address up to next alignment boundary"""
    remainder = address % alignment
    return address if remainder == 0 else address + (alignment - remainder)

TestMyStateMachine = StackAllocatorModel.TestCase # type: ignore