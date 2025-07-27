import ctypes
from ctypes import c_size_t
import hypothesis
from hypothesis.stateful import RuleBasedStateMachine, rule, precondition
from hypothesis.strategies import integers, sampled_from

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
Scratch Allocator Model
"""

@hypothesis.settings(max_examples=10000)
class StackAllocatorModel(RuleBasedStateMachine):
    def __init__(self):
        super().__init__()
        self.allocator = None
        self.is_destroyed = True

    def teardown(self):
        if self.allocator is not None:
            lib.anvil_memory_stack_allocator_destroy(ctypes.pointer(self.allocator))
        self.is_destroyed = True       

   
    @rule(
            exponent=integers(min_value=MIN_ALIGNMENT, max_value=MAX_ALIGNMENT),
            capacity=integers(min_value=64, max_value=(1 << 20)),  # Increased minimum capacity
            alloc_mode=sampled_from([EAGER, LAZY])
    )
    @precondition(lambda self: self.is_destroyed == True)
    def create_stack_allocator(self, exponent: int, capacity: int, alloc_mode: int):
        alignment = 1 << exponent

        self.allocator = lib.anvil_memory_stack_allocator_create(capacity, alignment, alloc_mode)
        self.is_destroyed = False

    @rule()
    @precondition(lambda self: self.is_destroyed == False)
    def destroy(self):
        if self.allocator is not None:
            allocator_ptr_ptr = ctypes.pointer(self.allocator)
            err = lib.anvil_memory_stack_allocator_destroy(allocator_ptr_ptr)
            self.allocator = None
            self.is_destroyed = True
            assert err == Error.SUCCESS, f"Allocator destruction failed with error code {err}"

TestMyStateMachine = StackAllocatorModel.TestCase # type: ignore