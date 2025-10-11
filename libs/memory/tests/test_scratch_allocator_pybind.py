"""Stateful Hypothesis tests validating the scratch allocator bindings."""

import anvil_memory as am
import ctypes
from ctypes import c_size_t, c_void_p
import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition, invariant
from hypothesis.strategies import integers, binary
from typing import Dict, List, Tuple

AllocationRecord = Tuple[object, int, int]
CopiedDataRecord = Tuple[bytes, int]

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
class ScratchAllocatorModel(RuleBasedStateMachine):
    """Hypothesis rule-based state machine exercising scratch allocator behavior."""
    def __init__(self):
        """Initialize the state machine with lifecycle tracking for allocator scenarios."""
        super().__init__()
        self.allocator = None
        self.allocations: List[AllocationRecord] = []
        self.copied_data: Dict[int, CopiedDataRecord] = {}
        self.is_destroyed = True
        self.capacity = 0
        self.external_allocations: List[int] = []
        self.allocator_id = 0

    def teardown(self):
        """Release outstanding resources allocated during test execution."""
        if self.allocator is not None:
            am.scratch_allocator_destroy(self.allocator)
        
        for ptr in self.external_allocations:
            if ptr:
                libc.free(ptr)
        self.allocations = []
        self.copied_data = {}
        self.external_allocations = []
        self.allocator_id = 0

    def _cleanup_stale_copied_data(self):
        """Discard copied-data records originating from a previous allocator lifecycle."""
        self.copied_data = {
            addr: (data, alloc_id) 
            for addr, (data, alloc_id) in self.copied_data.items() 
            if alloc_id == self.allocator_id
        }
   
    @rule(
            exponent=integers(min_value=am.MIN_ALIGNMENT_EXPONENT, max_value=am.MAX_ALIGNMENT_EXPONENT),
            capacity=integers(min_value=64, max_value=(1 << 20))
    )
    @precondition(lambda self: self.is_destroyed == True)
    def create_scratch_allocator(self, exponent: int, capacity: int):
        """Instantiate a fresh scratch allocator with the requested alignment and capacity."""
        alignment = 1 << exponent

        self.allocator = am.scratch_allocator_create(capacity, alignment)
        self.capacity = capacity
        self.is_destroyed = False
        self.allocator_id += 1
        self.copied_data = {}
        self.allocations = []

    @rule()
    @precondition(lambda self: self.is_destroyed == False)
    def destroy(self):
        """Dispose of the active scratch allocator and reset tracking structures."""
        if self.allocator is not None:
            err = am.scratch_allocator_destroy(self.allocator)
            self.allocator = None
            self.allocations = []
            self.copied_data = {}
            self.is_destroyed = True
            self.capacity = 0
            assert err == am.ERR_SUCCESS, f"Allocator destruction failed with error code {err}"

    @rule(
            alloc_size=integers(min_value=1, max_value=(1 << 20)),
            exponent=integers(min_value=am.MIN_ALIGNMENT_EXPONENT, max_value=am.MAX_ALIGNMENT_EXPONENT)
    )
    @precondition(lambda self: self.is_destroyed == False)
    def alloc(self, alloc_size: int, exponent: int):
        """Allocate scratch memory with the requested size and alignment."""
        if self.allocator is not None:
            alignment = 1 << exponent
            ptr = am.scratch_allocator_alloc(self.allocator, alloc_size, alignment)

            if ptr:
                self.allocations.append((ptr, alloc_size, alignment))

    @rule()
    @precondition(lambda self: self.is_destroyed == False)
    def allocator_reset(self):
        """Reset the allocator state and verify that future allocations start from a clean slate."""
        if self.allocator is not None:
            err = am.scratch_allocator_reset(self.allocator)
            self.allocations = []
            self.copied_data = {}
            assert err == am.ERR_SUCCESS, f"Allocator reset failed with error code {err}"

    @rule(data=binary(min_size=1, max_size=999999))
    @precondition(lambda self: self.is_destroyed == False) 
    def copy_data(self, data: bytes):
        """Copy external bytes into the allocator and ensure the copied payload matches the source."""
        if self.allocator is not None:
            src_size = len(data)
            
            dest_ptr = am.scratch_allocator_copy(
                self.allocator, data, src_size
            )
            
            if dest_ptr:
                dest_buffer = (ctypes.c_char * src_size).from_address(am.ptr_to_int(dest_ptr))
                copied_data = bytes(dest_buffer)
                assert copied_data == data, f"Data mismatch in copy: expected {data}, got {copied_data}"
                
                void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
                existing_alloc = next((alloc for alloc in self.allocations if alloc[0] == dest_ptr), None)
                assert existing_alloc is None, f"Address {dest_ptr} already tracked in allocations"
                
                self.allocations.append((dest_ptr, src_size, void_ptr_alignment))
                self.copied_data[am.ptr_to_int(dest_ptr)] = (data, self.allocator_id)

    @rule(data=binary(min_size=1, max_size=999999))   
    @precondition(lambda self: self.is_destroyed == False)
    def move_data(self, data: bytes):
        """Move external bytes into the allocator and verify ownership transfer semantics."""
        if self.allocator is not None:
            src_size = len(data)
            src_ptr = libc.malloc(src_size)
            
            if not src_ptr:
                return
            
            src_buffer = (ctypes.c_char * src_size).from_address(src_ptr)
            for i, byte in enumerate(data):
                src_buffer[i] = byte
            
            src_holder = ctypes.c_void_p(src_ptr)
            pointer_holder_addr = ctypes.addressof(src_holder)
            
            free_func_addr = ctypes.cast(libc.free, ctypes.c_void_p).value
            assert free_func_addr is not None, "Failed to resolve libc.free symbol address"
            
            dest_ptr = am.scratch_allocator_move(
                am.ptr_to_int(self.allocator),
                pointer_holder_addr,
                src_size,
                free_func_addr
            )
            
            if not dest_ptr:
                libc.free(src_ptr)
                return 

            dest_buffer = (ctypes.c_char * src_size).from_address(am.ptr_to_int(dest_ptr))
            moved_data = bytes(dest_buffer)
            assert moved_data == data, f"Data mismatch in move: expected {data}, got {moved_data}"
            
            assert src_holder.value is None or src_holder.value == 0, (
                "Source pointer should be NULL after move"
            )
            
            if src_ptr in self.external_allocations:
                self.external_allocations.remove(src_ptr)
            
            void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
            self.allocations.append((dest_ptr, src_size, void_ptr_alignment))
            self.copied_data[am.ptr_to_int(dest_ptr)] = (data, self.allocator_id)

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) >= 1)
    def inv_no_alloc_overlap(self):
        """Ensure recorded allocation ranges do not overlap within a lifecycle."""
        for i, (address, size, _) in enumerate(self.allocations):
            for (address2, size2, _) in self.allocations[i+1:]:
                addr1 = am.ptr_to_int(address)
                addr2 = am.ptr_to_int(address2)
                if addr1 < addr2 + size2 and addr2 < addr1 + size:
                    assert False, f"Memory allocations overlap: allocation at {addr1} (size {size}) overlaps with allocation at {addr2} (size {size2})"
    
    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) >= 1)
    def inv_allocations_are_contiguous(self):
        """Require each allocation to touch the next allocation once alignment padding is applied."""
        for i, (address, size, _) in enumerate(self.allocations):
            if i == len(self.allocations) - 1:
                break

            next_address, _, next_alignment = self.allocations[i + 1]
        
            current_end = am.ptr_to_int(address) + size
        
            expected_next_start = align_up(current_end, next_alignment)
        
            assert expected_next_start == am.ptr_to_int(next_address), f"Allocations not contiguous: expected {expected_next_start}, got {am.ptr_to_int(next_address)}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) > 0)
    def inv_allocations_properly_aligned(self):
        """Confirm allocator addresses respect their recorded alignment requirements."""
        for address, _, alignment in self.allocations:
            if am.ptr_to_int(address) % alignment != 0:
                assert False, f"Address {am.ptr_to_int(address)} not aligned to {alignment}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) > 0)
    def inv_allocations_within_bounds(self):
        """Guarantee cumulative allocations never exceed the configured capacity."""
        first_addr = self.allocations[0][0]
        last_addr, last_size, _ = self.allocations[-1]
        total_used = (am.ptr_to_int(last_addr) + last_size) - am.ptr_to_int(first_addr)
        
        assert total_used <= self.capacity, f"Used {total_used} bytes exceeds capacity {self.capacity}"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_power_of_two_alignments(self):
        """Verify each recorded alignment is a valid power of two."""
        for _, _, alignment in self.allocations:
            if alignment <= 0 or (alignment & (alignment - 1)) != 0:
                assert False, f"Alignment {alignment} is not a power of 2"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_positive_sizes(self):
        """Confirm all allocations carry a strictly positive size."""
        for _, size, _ in self.allocations:
            if size <= 0:
                assert False, f"Invalid allocation size: {size}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False)
    def inv_copied_data_integrity(self):
        """Validate that copied or moved data remains intact for the current allocator instance."""
        self._cleanup_stale_copied_data()
        
        for address, (original_data, data_allocator_id) in self.copied_data.items():
            if data_allocator_id != self.allocator_id:
                continue
                
            allocation = next((alloc for alloc in self.allocations if alloc[0] == address), None)
            if allocation:
                addr, size, _ = allocation
                
                expected_size = len(original_data)
                assert size == expected_size, \
                    f"Size mismatch at {addr}: allocation size {size} != original data size {expected_size}"
                
                buffer = (ctypes.c_char * expected_size).from_address(am.ptr_to_int(addr))
                current_data = bytes(buffer)
                assert current_data == original_data, \
                    f"Data corruption detected at {addr}: expected {original_data}, got {current_data}"

def align_up(address: int, alignment: int): 
    """Return the smallest aligned address greater than or equal to ``address``."""
    remainder = address % alignment
    return address if remainder == 0 else address + (alignment - remainder)

TestMyStateMachine = ScratchAllocatorModel.TestCase # type: ignore
