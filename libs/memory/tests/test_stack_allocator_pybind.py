"""Stateful Hypothesis tests validating the stack allocator bindings."""

import anvil_memory as am
import ctypes
from ctypes import c_size_t, c_void_p
import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition, invariant
from hypothesis.strategies import integers, binary, sampled_from
from typing import Dict, List, Tuple

AllocationRecord = Tuple[object, int, int, int]
CopiedDataRecord = Tuple[bytes, int]
RecordSnapshot = Tuple[int, int]
libc = ctypes.CDLL("libc.so.6")
libc.malloc.argtypes = [c_size_t]
libc.malloc.restype = c_void_p
libc.free.argtypes = [c_void_p]
libc.free.restype = None
libc.memcmp.argtypes = [c_void_p, c_void_p, c_size_t]
libc.memcmp.restype = ctypes.c_int

@hypothesis.settings(max_examples=100, stateful_step_count=10,  # Reduce steps per example
    deadline=None,  # Disable per-test deadlines
    suppress_health_check=[hypothesis.HealthCheck.too_slow])
class StackAllocatorModel(RuleBasedStateMachine):
    """Hypothesis state machine that exercises stack allocator lifecycles and invariants."""
    def __init__(self):
        """Initialize tracking structures for allocator lifetimes and allocation epochs."""
        super().__init__()
        self.allocator = None
        self.allocations: List[AllocationRecord] = []
        self.copied_data: Dict[int, CopiedDataRecord] = {}
        self.is_destroyed = True
        self.capacity = 0
        self.alloc_mode = am.EAGER
        self.external_allocations: List[int] = []
        self.allocator_id = 0
        self.top_alignment = 0
        self.allocation_epoch = 0
        self.top_ptr = None
        self.stack: List[RecordSnapshot] = []

    def teardown(self):
        """Release outstanding resources allocated during test execution."""
        if self.allocator is not None:
            am.stack_allocator_destroy(self.allocator)
        
        for ptr in self.external_allocations:
            if ptr:
                libc.free(ptr)
        self.allocations = []
        self.copied_data = {}
        self.external_allocations = []
        self.allocator_id = 0
        self.allocation_epoch = 0

    def _cleanup_stale_copied_data(self):
        """Discard copied-data records originating from prior allocator lifecycles."""
        self.copied_data = {
            addr: (data, alloc_id) 
            for addr, (data, alloc_id) in self.copied_data.items() 
            if alloc_id == self.allocator_id
        }
   
    @rule(
            exponent=integers(min_value=am.MIN_ALIGNMENT_EXPONENT, max_value=am.MAX_ALIGNMENT_EXPONENT),
            capacity=integers(min_value=64, max_value=(1 << 20)),
            alloc_mode=sampled_from([am.EAGER, am.LAZY])
    )
    @precondition(lambda self: self.is_destroyed == True)
    def create_stack_allocator(self, exponent: int, capacity: int, alloc_mode: int):
        """Instantiate a stack allocator with the requested alignment, capacity, and mode."""
        alignment = 1 << exponent

        self.allocator = am.stack_allocator_create(capacity, alignment, alloc_mode)
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
    @precondition(lambda self: self.is_destroyed == False)
    def destroy(self):
        """Dispose of the active stack allocator and clear bookkeeping."""
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
            assert err == am.ERR_SUCCESS, f"Allocator destruction failed with error code {err}"

    @rule(
            alloc_size=integers(min_value=1, max_value=(1 << 20)),
            exponent=integers(min_value=am.MIN_ALIGNMENT_EXPONENT, max_value=am.MAX_ALIGNMENT_EXPONENT)
    )
    @precondition(lambda self: self.is_destroyed == False)
    def alloc(self, alloc_size: int, exponent: int):
        """Allocate scratch memory from the stack allocator and track the call epoch."""
        if self.allocator is not None:
            alignment = 1 << exponent
            ptr = am.stack_allocator_alloc(self.allocator, alloc_size, alignment)

            if ptr:
                self.allocations.append((ptr, alloc_size, alignment, self.allocation_epoch))
            if ptr and self.top_ptr is not None:
                expected_addr = align_up(self.top_ptr, alignment)
                assert am.ptr_to_int(ptr) == expected_addr, f"Expected {expected_addr}, got {am.ptr_to_int(ptr)}"
                self.top_ptr = None

    @rule()
    @precondition(lambda self: self.is_destroyed == False)
    def allocator_reset(self):
        """Reset allocator state and ensure the internal tracking is cleared."""
        if self.allocator is not None:
            err = am.stack_allocator_reset(self.allocator)
            self.allocations = []
            self.copied_data = {}
            self.stack = []
            self.top_ptr = None
            self.allocation_epoch = 0
            assert err == am.ERR_SUCCESS, f"Allocator reset failed with error code {err}"

    @rule(data=binary(min_size=1, max_size=999999))
    @precondition(lambda self: self.is_destroyed == False) 
    def copy_data(self, data: bytes):
        """Copy external bytes into the allocator and verify the stored payload matches the source."""
        if self.allocator is not None:
            src_size = len(data)
            
            dest_ptr = am.stack_allocator_copy(
                self.allocator, data, src_size
            )
            
            if dest_ptr:
                dest_buffer = (ctypes.c_char * src_size).from_address(am.ptr_to_int(dest_ptr))
                copied_data = bytes(dest_buffer)
                assert copied_data == data, f"Data mismatch in copy: expected {data}, got {copied_data}"
                
                void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
                existing_alloc = next((alloc for alloc in self.allocations if alloc[0] == dest_ptr), None)
                assert existing_alloc is None, f"Address {am.ptr_to_int(dest_ptr)} already tracked in allocations"
                
                self.allocations.append((dest_ptr, src_size, void_ptr_alignment, self.allocation_epoch))
                self.copied_data[am.ptr_to_int(dest_ptr)] = (data, self.allocator_id)
            
            if dest_ptr and self.top_ptr is not None:
                void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
                expected_min_addr = align_up(self.top_ptr, void_ptr_alignment)
                assert am.ptr_to_int(dest_ptr) >= expected_min_addr, f"Address {am.ptr_to_int(dest_ptr)} comes before expected minimum {expected_min_addr}"
                assert am.ptr_to_int(dest_ptr) % void_ptr_alignment == 0, f"Address {am.ptr_to_int(dest_ptr)} not properly aligned to {void_ptr_alignment}"
                self.top_ptr = None

    @rule(data=binary(min_size=1, max_size=999999))   
    @precondition(lambda self: self.is_destroyed == False)
    def move_data(self, data: bytes):
        """Move external bytes into allocator storage while verifying ownership transfer semantics."""
        if self.allocator is not None:
            src_size = len(data)
            src_ptr = libc.malloc(src_size)
            
            if not src_ptr:
                return  
            
            src_buffer = (ctypes.c_char * src_size).from_address(src_ptr)
            for i, byte in enumerate(data):
                src_buffer[i] = byte
            
            src_holder = ctypes.c_void_p(src_ptr)
            src_ptr_addr = ctypes.addressof(src_holder)
            
            free_func = ctypes.cast(libc.free, ctypes.c_void_p).value
            
            if not free_func:
                assert False, "free function doesn't exist"

            dest_ptr = am.stack_allocator_move(
                am.ptr_to_int(self.allocator), src_ptr_addr, src_size, free_func
            )
            
            if not dest_ptr:
                libc.free(src_ptr)
                return 

            dest_buffer = (ctypes.c_char * src_size).from_address(am.ptr_to_int(dest_ptr))
            moved_data = bytes(dest_buffer)
            assert moved_data == data, f"Data mismatch in move: expected {data}, got {moved_data}"
            
            assert src_holder.value is None or src_holder.value == 0, \
                "Source pointer should be nullptr after move"
            
            if src_ptr in self.external_allocations:
                self.external_allocations.remove(src_ptr)
            
            void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
            
            existing_alloc = next((alloc for alloc in self.allocations if alloc[0] == dest_ptr), None)
            assert existing_alloc is None, f"Address {am.ptr_to_int(dest_ptr)} already tracked in allocations"
            
            self.allocations.append((dest_ptr, src_size, void_ptr_alignment, self.allocation_epoch))
            self.copied_data[am.ptr_to_int(dest_ptr)] = (data, self.allocator_id)
        
            if dest_ptr and self.top_ptr is not None:
                void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
                expected_min_addr = align_up(self.top_ptr, void_ptr_alignment)
                assert am.ptr_to_int(dest_ptr) >= expected_min_addr, f"Address {am.ptr_to_int(dest_ptr)} comes before expected minimum {expected_min_addr}"
                assert am.ptr_to_int(dest_ptr) % void_ptr_alignment == 0, f"Address {am.ptr_to_int(dest_ptr)} not properly aligned to {void_ptr_alignment}"
                self.top_ptr = None

    @rule()   
    @precondition(lambda self: self.is_destroyed == False and 1 <= len(self.allocations) <= 50)
    def record(self):
        """Capture the allocator's current allocation totals for later unwind verification."""
        am.stack_allocator_record(self.allocator)
        current_allocation_count = len(self.allocations)
        if current_allocation_count > 0:
            first_addr = self.allocations[0][0]
            last_addr, last_size, _, _ = self.allocations[-1]
            total_allocated = (am.ptr_to_int(last_addr) + last_size) - am.ptr_to_int(first_addr)
            self.stack.append((total_allocated, current_allocation_count))
        else:
            self.stack.append((0, 0))

    @rule()   
    @precondition(lambda self: self.is_destroyed == False and len(self.stack) > 0)
    def unwind(self):
        """Restore allocator state to the most recent record and drop newer allocations."""
        am.stack_allocator_unwind(self.allocator)

        _, allocation_count = self.stack.pop()
        self.allocations = self.allocations[:allocation_count]
        self.allocation_epoch += 1
        remaining_addresses = {addr for addr, _, _, _ in self.allocations}
        self.copied_data = {
            addr: (data, alloc_id) 
            for addr, (data, alloc_id) in self.copied_data.items() 
            if addr in remaining_addresses and alloc_id == self.allocator_id
        }
        if self.allocations:
            last_addr, last_size, _, _ = self.allocations[-1]
            self.top_ptr = am.ptr_to_int(last_addr) + last_size
        else:
            self.top_ptr = None

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) >= 1)
    def inv_no_alloc_overlap(self):
        """Ensure individual allocations never overlap in address space."""
        for i, (address, size, _, _) in enumerate(self.allocations):
            for (address2, size2, _, _) in self.allocations[i+1:]:
                addr1 = am.ptr_to_int(address)
                addr2 = am.ptr_to_int(address2)
                if addr1 < addr2 + size2 and addr2 < addr1 + size:
                    assert False, f"Memory allocations overlap: allocation at {addr1} (size {size}) overlaps with allocation at {addr2} (size {size2})"
    
    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) >= 2)
    def inv_allocations_are_contiguous(self):
        """Verify allocations in the same epoch remain contiguous despite unwind operations."""
        for i in range(len(self.allocations) - 1):
            current_addr, current_size, _, current_epoch = self.allocations[i]
            next_addr, _, next_alignment, next_epoch = self.allocations[i + 1]
            
            if current_epoch == next_epoch:
                current_end = am.ptr_to_int(current_addr) + current_size
                expected_next_start = align_up(current_end, next_alignment)
                
                assert am.ptr_to_int(next_addr) >= expected_next_start, \
                    f"Allocations in same epoch not contiguous: expected >= {expected_next_start}, got {am.ptr_to_int(next_addr)}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) > 0)
    def inv_allocations_properly_aligned(self):
        """Confirm each allocation respects its requested alignment."""
        for address, _, alignment, _ in self.allocations:
            if am.ptr_to_int(address) % alignment != 0:
                assert False, f"Address {address} not aligned to {alignment}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) > 0)
    def inv_allocations_within_bounds(self):
        """Check that total allocated bytes never exceed allocator capacity."""
        first_addr = self.allocations[0][0]
        last_addr, last_size, _, _ = self.allocations[-1]
        total_used = (am.ptr_to_int(last_addr) + last_size) - am.ptr_to_int(first_addr)
        
        assert total_used <= self.capacity, f"Used {total_used} bytes exceeds capacity {self.capacity}"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_power_of_two_alignments(self):
        """Assert that every allocation alignment is a power of two."""
        for _, _, alignment, _ in self.allocations:
            if alignment <= 0 or (alignment & (alignment - 1)) != 0:
                assert False, f"Alignment {alignment} is not a power of 2"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_positive_sizes(self):
        """Guarantee that each allocation requests a strictly positive size."""
        for _, size, _, _ in self.allocations:
            if size <= 0:
                assert False, f"Invalid allocation size: {size}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False)
    def inv_copied_data_integrity(self):
        """Verify that copied or moved data remains byte-for-byte identical to its source."""
        self._cleanup_stale_copied_data()
        
        for address, (original_data, data_allocator_id) in self.copied_data.items():
            if data_allocator_id != self.allocator_id:
                continue
                
            allocation = next((alloc for alloc in self.allocations if alloc[0] == address), None)
            if allocation:
                addr, size, _, _ = allocation
                
                expected_size = len(original_data)
                assert size == expected_size, \
                    f"Size mismatch at {addr}: allocation size {size} != original data size {expected_size}"
                
                buffer = (ctypes.c_char * expected_size).from_address(am.ptr_to_int(addr))
                current_data = bytes(buffer)
                assert current_data == original_data, \
                    f"Data corruption detected at {addr}: expected {original_data}, got {current_data}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and self.alloc_mode == am.LAZY and len(self.allocations) > 0)
    def inv_lazy_allocation_behavior(self):
        """Placeholder invariant surface for future lazy-allocation commitment checks."""
        pass

def align_up(address: int, alignment: int): 
    """Align an address up to the next alignment boundary."""
    remainder = address % alignment
    return address if remainder == 0 else address + (alignment - remainder)

TestMyStateMachine = StackAllocatorModel.TestCase # type: ignore
