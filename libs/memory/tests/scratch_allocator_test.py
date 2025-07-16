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
        self.allocations: List[tuple[int,int]] = []
        self.is_destroyed = True

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
                self.allocations.append((ptr, alloc_size))

    @rule()
    @precondition(lambda self: self.is_destroyed == False)
    def allocator_reset(self):
        if self.allocator is not None:
            err = lib.anvil_memory_scratch_allocator_reset(self.allocator)
            self.allocations = []
            assert err == Error.SUCCESS

    @invariant()
    def inv_no_alloc_overlap(self):
        if len(self.allocations) <= 1:
            assert True
        
        for i, (address, size) in enumerate(self.allocations):
            for address2, size2 in self.allocations[i+1:]:
                if address < address2 + size2 and address2 < address + size:
                    assert False
        
        assert True


   
TestMyStateMachine = ScratchAllocatorModel.TestCase # type: ignore