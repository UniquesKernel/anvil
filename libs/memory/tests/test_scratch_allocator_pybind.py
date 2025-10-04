import anvil_memory as am
import ctypes
from ctypes import c_size_t, c_void_p
import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition, invariant
from hypothesis.strategies import integers, binary
from typing import List, Dict

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
Scratch Allocator Model
"""

@hypothesis.settings(max_examples=1000)
class ScratchAllocatorModel(RuleBasedStateMachine):
    def __init__(self):
        super().__init__()
        self.allocator = None
        self.allocations: List[tuple[object, int, int]] = []  # (pointer as object, size, alignment)
        self.copied_data: Dict[int, tuple[bytes, int]] = {}  # address -> (original data, allocator_id)
        self.is_destroyed = True
        self.capacity = 0
        self.external_allocations: List[int] = []  # Track malloc'd pointers for cleanup
        self.allocator_id = 0  # Track allocator lifecycle

    def teardown(self):
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
        """Remove copied_data entries that don't belong to current allocator lifecycle"""
        self.copied_data = {
            addr: (data, alloc_id) 
            for addr, (data, alloc_id) in self.copied_data.items() 
            if alloc_id == self.allocator_id
        }
   
    @rule(
            exponent=integers(min_value=am.MIN_ALIGNMENT_EXPONENT, max_value=am.MAX_ALIGNMENT_EXPONENT),
            capacity=integers(min_value=64, max_value=(1 << 20))  # Increased minimum capacity
    )
    @precondition(lambda self: self.is_destroyed == True)
    def create_scratch_allocator(self, exponent: int, capacity: int):
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
        if self.allocator is not None:
            err = am.scratch_allocator_destroy(self.allocator)
            self.allocator = None
            self.allocations: list[tuple[object, int, int]] = []
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
        if self.allocator is not None:
            alignment = 1 << exponent
            ptr = am.scratch_allocator_alloc(self.allocator, alloc_size, alignment)

            if ptr:
                self.allocations.append((ptr, alloc_size, alignment))

    @rule()
    @precondition(lambda self: self.is_destroyed == False)
    def allocator_reset(self):
        if self.allocator is not None:
            err = am.scratch_allocator_reset(self.allocator)
            self.allocations = []
            self.copied_data = {}
            assert err == am.ERR_SUCCESS, f"Allocator reset failed with error code {err}"

    @rule(data=binary(min_size=1, max_size=999999))
    @precondition(lambda self: self.is_destroyed == False) 
    def copy_data(self, data: bytes):
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
                
                # Defensive check: ensure we're not tracking duplicate addresses
                existing_alloc = next((alloc for alloc in self.allocations if alloc[0] == dest_ptr), None)
                assert existing_alloc is None, f"Address {dest_ptr} already tracked in allocations"
                
                self.allocations.append((dest_ptr, src_size, void_ptr_alignment))
                self.copied_data[am.ptr_to_int(dest_ptr)] = (data, self.allocator_id)

    @rule(data=binary(min_size=1, max_size=999999))   
    @precondition(lambda self: self.is_destroyed == False)
    def move_data(self, data: bytes):
        if self.allocator is not None:
            src_size = len(data)
            src_ptr = libc.malloc(src_size)
            
            if not src_ptr:
                return  
            
            src_buffer = (ctypes.c_char * src_size).from_address(src_ptr)
            for i, byte in enumerate(data):
                src_buffer[i] = byte
            
            # Create a pointer holder and get its address
            src_holder = ctypes.c_void_p(src_ptr)
            src_ptr_addr = ctypes.addressof(src_holder)
            
            free_func_addr = ctypes.cast(libc.free, ctypes.c_void_p).value
            
            dest_ptr = am.scratch_allocator_move(
                am.ptr_to_int(self.allocator),
                src_ptr_addr,  # Address of the pointer holder
                src_size,
                free_func_addr
            )
            
            if not dest_ptr:
                libc.free(src_ptr)
                return 

            dest_buffer = (ctypes.c_char * src_size).from_address(am.ptr_to_int(dest_ptr))
            moved_data = bytes(dest_buffer)
            assert moved_data == data, f"Data mismatch in move: expected {data}, got {moved_data}"
            
            # Check that src_holder was set to NULL by the C function
            assert src_holder.value is None or src_holder.value == 0, \
                "Source pointer should be NULL after move"
            
            if src_ptr in self.external_allocations:
                self.external_allocations.remove(src_ptr)
            
            void_ptr_alignment = ctypes.sizeof(ctypes.c_void_p)
            self.allocations.append((dest_ptr, src_size, void_ptr_alignment))
            self.copied_data[am.ptr_to_int(dest_ptr)] = (data, self.allocator_id)

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) >= 1)
    def inv_no_alloc_overlap(self):
        for i, (address, size, _) in enumerate(self.allocations):
            for (address2, size2, _) in self.allocations[i+1:]:
                addr1 = am.ptr_to_int(address)
                addr2 = am.ptr_to_int(address2)
                if addr1 < addr2 + size2 and addr2 < addr1 + size:
                    assert False, f"Memory allocations overlap: allocation at {add1} (size {size}) overlaps with allocation at {addr2} (size {size2})"
    
    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) >= 1)
    def inv_allocations_are_contiguous(self):
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
        for address, _, alignment in self.allocations:
            if am.ptr_to_int(address) % alignment != 0:
                assert False, f"Address {am.ptr_to_int(address)} not aligned to {alignment}"

    @invariant()
    @precondition(lambda self: self.is_destroyed == False and len(self.allocations) > 0)
    def inv_allocations_within_bounds(self):
        first_addr = self.allocations[0][0]
        last_addr, last_size, _ = self.allocations[-1]
        total_used = (am.ptr_to_int(last_addr) + last_size) - am.ptr_to_int(first_addr)
        
        assert total_used <= self.capacity, f"Used {total_used} bytes exceeds capacity {self.capacity}"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_power_of_two_alignments(self):
        for _, _, alignment in self.allocations:
            if alignment <= 0 or (alignment & (alignment - 1)) != 0:
                assert False, f"Alignment {alignment} is not a power of 2"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_positive_sizes(self):
        for _, size, _ in self.allocations:
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
                addr, size, _ = allocation
                
                expected_size = len(original_data)
                assert size == expected_size, \
                    f"Size mismatch at {addr}: allocation size {size} != original data size {expected_size}"
                
                buffer = (ctypes.c_char * expected_size).from_address(am.ptr_to_int(addr))
                current_data = bytes(buffer)
                assert current_data == original_data, \
                    f"Data corruption detected at {addr}: expected {original_data}, got {current_data}"

def align_up(address: int, alignment: int): 
    """Align address up to next alignment boundary"""
    remainder = address % alignment
    return address if remainder == 0 else address + (alignment - remainder)

TestMyStateMachine = ScratchAllocatorModel.TestCase # type: ignore