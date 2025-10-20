"""Stateful Hypothesis tests validating the scratch allocator bindings (simplified)."""

import ctypes
from ctypes import c_size_t, c_void_p
from typing import List, Tuple, Dict

import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition, invariant
from hypothesis.strategies import integers, binary

import anvil_memory as am

# Records: (address:int, size:int, alignment:int)
AllocationRecord = Tuple[int, int, int]
PTR_ALIGN = ctypes.sizeof(ctypes.c_void_p)

# libc setup
libc = ctypes.CDLL("libc.so.6")
libc.malloc.argtypes = [c_size_t]
libc.malloc.restype = c_void_p
libc.free.argtypes = [c_void_p]
libc.free.restype = None


def align_up(address: int, alignment: int) -> int:
    """Smallest aligned address >= address."""
    rem = address % alignment
    return address if rem == 0 else address + (alignment - rem)


@hypothesis.settings(
    max_examples=100,
    stateful_step_count=10,  # fewer steps per example
    deadline=None,           # disable per-test deadlines
    suppress_health_check=[hypothesis.HealthCheck.too_slow],
)
class ScratchAllocatorModel(RuleBasedStateMachine):
    """Rule-based model exercising scratch allocator behavior."""

    def __init__(self):
        super().__init__()
        self.allocator = None  # py::capsule or None
        self.capacity = 0
        self.allocations: List[AllocationRecord] = []
        # Map: address(int) -> original bytes
        self.copied_data: Dict[int, bytes] = {}

    # ------------- Lifecycle -------------

    @rule(
        exponent=integers(min_value=am.MIN_ALIGNMENT_EXPONENT, max_value=am.MAX_ALIGNMENT_EXPONENT),
        capacity=integers(min_value=64, max_value=(1 << 20)),
    )
    @precondition(lambda self: self.allocator is None)
    def create_scratch_allocator(self, exponent: int, capacity: int):
        alignment = 1 << exponent
        self.allocator = am.scratch_allocator_create(capacity, alignment)
        self.capacity = capacity
        self.allocations.clear()
        self.copied_data.clear()

    @rule()
    @precondition(lambda self: self.allocator is not None)
    def destroy(self):
        err = am.scratch_allocator_destroy(self.allocator)
        self.allocator = None
        self.capacity = 0
        self.allocations.clear()
        self.copied_data.clear()
        assert err == am.ERR_SUCCESS, f"Allocator destruction failed with error code {err}"

    @rule()
    @precondition(lambda self: self.allocator is not None)
    def allocator_reset(self):
        err = am.scratch_allocator_reset(self.allocator)
        self.allocations.clear()
        self.copied_data.clear()
        assert err == am.ERR_SUCCESS, f"Allocator reset failed with error code {err}"

    # ------------- Operations -------------

    @rule(
        alloc_size=integers(min_value=1, max_value=(1 << 20)),
        exponent=integers(min_value=am.MIN_ALIGNMENT_EXPONENT, max_value=am.MAX_ALIGNMENT_EXPONENT),
    )
    @precondition(lambda self: self.allocator is not None)
    def alloc(self, alloc_size: int, exponent: int):
        alignment = 1 << exponent
        ptr = am.scratch_allocator_alloc(self.allocator, alloc_size, alignment)
        if ptr:
            addr = am.ptr_to_int(ptr)
            self.allocations.append((addr, alloc_size, alignment))

    @rule(data=binary(min_size=1, max_size=999_999))
    @precondition(lambda self: self.allocator is not None)
    def copy_data(self, data: bytes):
        n = len(data)
        dest_ptr = am.scratch_allocator_copy(self.allocator, data, n)
        if not dest_ptr:
            return

        addr = am.ptr_to_int(dest_ptr)
        # Validate the bytes were copied correctly
        buf = (ctypes.c_char * n).from_address(addr)
        assert bytes(buf) == data, "Data mismatch after copy"

        # Ensure we don't double-track the same address
        assert all(a != addr for a, _, _ in self.allocations), f"Address {addr} already tracked"
        self.allocations.append((addr, n, PTR_ALIGN))
        self.copied_data[addr] = data

    @rule(data=binary(min_size=1, max_size=999_999))
    @precondition(lambda self: self.allocator is not None)
    def move_data(self, data: bytes):
        n = len(data)
        src_ptr = libc.malloc(n)
        if not src_ptr:
            return

        # Write source bytes
        src_buf = (ctypes.c_char * n).from_address(src_ptr)
        src_buf[:] = data

        # Holder for pointer (so C++ can null it out)
        src_holder = ctypes.c_void_p(src_ptr)
        holder_addr = ctypes.addressof(src_holder)
        free_addr = ctypes.cast(libc.free, ctypes.c_void_p).value
        assert free_addr is not None, "Failed to resolve libc.free"

        dest_ptr = am.scratch_allocator_move(
            am.ptr_to_int(self.allocator), holder_addr, n, free_addr
        )
        if not dest_ptr:
            libc.free(src_ptr)
            return

        addr = am.ptr_to_int(dest_ptr)
        moved = (ctypes.c_char * n).from_address(addr)
        assert bytes(moved) == data, "Data mismatch after move"
        assert not src_holder.value, "Source pointer should be nullptr after move"

        self.allocations.append((addr, n, PTR_ALIGN))
        self.copied_data[addr] = data

    # ------------- Invariants -------------

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) >= 1)
    def inv_no_alloc_overlap(self):
        for i, (a1, s1, _) in enumerate(self.allocations):
            for a2, s2, _ in self.allocations[i + 1 :]:
                if a1 < a2 + s2 and a2 < a1 + s1:
                    assert False, f"Overlap: [{a1},{a1+s1}) with [{a2},{a2+s2})"

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) >= 1)
    def inv_allocations_are_contiguous(self):
        for i, (addr, size, _) in enumerate(self.allocations[:-1]):
            next_addr, _, next_align = self.allocations[i + 1]
            expected = align_up(addr + size, next_align)
            assert expected == next_addr, f"Not contiguous: expected {expected}, got {next_addr}"

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) > 0)
    def inv_allocations_properly_aligned(self):
        for addr, _, align in self.allocations:
            assert addr % align == 0, f"Address {addr} not aligned to {align}"

    @invariant()
    @precondition(lambda self: self.allocator is not None and len(self.allocations) > 0)
    def inv_allocations_within_bounds(self):
        first_addr = self.allocations[0][0]
        last_addr, last_size, _ = self.allocations[-1]
        used = (last_addr + last_size) - first_addr
        assert used <= self.capacity, f"Used {used} > capacity {self.capacity}"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_power_of_two_alignments(self):
        for _, _, align in self.allocations:
            assert align > 0 and (align & (align - 1)) == 0, f"Alignment {align} not power-of-two"

    @invariant()
    @precondition(lambda self: len(self.allocations) > 0)
    def inv_positive_sizes(self):
        for _, size, _ in self.allocations:
            assert size > 0, f"Invalid allocation size: {size}"

    @invariant()
    @precondition(lambda self: self.allocator is not None)
    def inv_copied_data_integrity(self):
        for addr, original in self.copied_data.items():
            # Find matching allocation (if any)
            found = next((rec for rec in self.allocations if rec[0] == addr), None)
            if not found:
                continue
            _, size, _ = found
            assert size == len(original), f"Size mismatch at {addr}: {size} != {len(original)}"
            buf = (ctypes.c_char * size).from_address(addr)
            assert bytes(buf) == original, f"Data corruption at {addr}"

# For pytest discovery
TestMyStateMachine = ScratchAllocatorModel.TestCase  # type: ignore
