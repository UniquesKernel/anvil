from anvil_memory.lazy_stack_allocator import (
    lazy_stack_allocator_alloc,
    lazy_stack_allocator_create,
    lazy_stack_allocator_destroy,
    lazy_stack_allocator_record,
    lazy_stack_allocator_reset,
    lazy_stack_allocator_unwind,
)
from anvil_memory.stack_allocator import (
    stack_allocator_alloc,
    stack_allocator_create,
    stack_allocator_destroy,
    stack_allocator_record,
    stack_allocator_reset,
    stack_allocator_unwind,
)
from anvil_memory import Error, MAX_STACK_DEPTH, ptr_to_int
from hypothesis import strategies as st, reproduce_failure
from hypothesis.stateful import RuleBasedStateMachine, initialize, invariant, precondition, rule


@st.composite
def allocator_config(draw):
    """Generate (capacity, alignment) where alignment is power-of-two in supported range."""
    base = draw(st.integers(min_value=3, max_value=11))
    alignment = 2 ** base

    capacity = draw(st.integers(min_value=1, max_value=2**24))
    return (capacity, alignment)

class DifferentialAllocatorModel(RuleBasedStateMachine):
    """Differential testing: lazy-stack and stack allocators should behave identically."""

    def __init__(self):
        super().__init__()
        self.lazy_allocator = None
        self.stack_allocator = None
        self.isValid = False
        self.capacity = 0
        self.allocated = 0
        self.epoch = 0
        self.lazy_allocations = []
        self.stack_allocations = []
        self.stack = []

    @initialize(config=allocator_config())
    def initialize_model(self, config):
        capacity, alignment = config

        lazy_err, lazy_allocator = lazy_stack_allocator_create(capacity, alignment)
        stack_err, stack_allocator = stack_allocator_create(capacity, alignment)

        assert lazy_err == stack_err
        assert (lazy_allocator is None) == (stack_allocator is None)

        self.lazy_allocator = lazy_allocator
        self.stack_allocator = stack_allocator
        self.isValid = True
        self.capacity = capacity
        self.allocated = 0
        self.epoch = 0
        self.lazy_allocations.clear()
        self.stack_allocations.clear()
        self.stack.clear()

    @rule(config=allocator_config())
    @precondition(lambda self: self.isValid == False)
    def create(self, config):
        capacity, alignment = config

        lazy_err, lazy_allocator = lazy_stack_allocator_create(capacity, alignment)
        stack_err, stack_allocator = stack_allocator_create(capacity, alignment)

        assert lazy_err == stack_err
        assert (lazy_allocator is not None) == (stack_allocator is not None)

        self.lazy_allocator = lazy_allocator
        self.stack_allocator = stack_allocator
        self.isValid = True
        self.capacity = capacity
        self.allocated = 0
        self.epoch = 0
        self.lazy_allocations.clear()
        self.stack_allocations.clear()
        self.stack.clear()

    @rule(config=allocator_config())
    def alloc(self, config):
        allocation_size, alignment = config

        lazy_ptr, _ = lazy_stack_allocator_alloc(self.lazy_allocator, allocation_size, alignment)
        stack_ptr, _ = stack_allocator_alloc(self.stack_allocator, allocation_size, alignment)

        assert (lazy_ptr is None) == (stack_ptr is None)

        if lazy_ptr and stack_ptr:
            self.lazy_allocations.append((lazy_ptr, allocation_size, alignment, self.epoch))
            self.stack_allocations.append((stack_ptr, allocation_size, alignment, self.epoch))
            self.allocated = self.allocated + allocation_size

    @rule()
    def allocator_reset(self):
        lazy_err = lazy_stack_allocator_reset(self.lazy_allocator)
        stack_err = stack_allocator_reset(self.stack_allocator)

        if (self.isValid == True):
            assert lazy_err == stack_err
            self.lazy_allocations.clear()
            self.stack_allocations.clear()
            self.stack.clear()
            self.allocated = 0
            self.epoch = 0
        else:
            assert lazy_err == stack_err 

    @rule()
    def record(self):
        lazy_err = lazy_stack_allocator_record(self.lazy_allocator)
        stack_err = stack_allocator_record(self.stack_allocator)

        if (self.isValid == False):
            assert lazy_err == stack_err
            return

        if (len(self.lazy_allocations) > MAX_STACK_DEPTH):
            assert lazy_err == stack_err
            return

        assert lazy_err == stack_err

        epoch_alloc_size = sum([x for (_, x, _, epoch) in self.lazy_allocations if epoch == self.epoch])
        self.stack.append((len(self.lazy_allocations), epoch_alloc_size))
        self.epoch += 1

    @rule()
    def unwind(self):
        lazy_err = lazy_stack_allocator_unwind(self.lazy_allocator)
        stack_err = stack_allocator_unwind(self.stack_allocator)

        if (self.isValid == False):
            assert lazy_err == stack_err
            return

        if (len(self.stack) == 0):
            assert lazy_err == stack_err
            return

        assert lazy_err == stack_err

        allocation_count, epoch_size = self.stack.pop()
        self.lazy_allocations = self.lazy_allocations[:allocation_count]
        self.stack_allocations = self.stack_allocations[:allocation_count]
        self.epoch -= 1
        self.allocated -= epoch_size

    @rule()
    def destroy(self):
        lazy_err = lazy_stack_allocator_destroy(self.lazy_allocator)
        stack_err = stack_allocator_destroy(self.stack_allocator)

        if (self.isValid == True):
            assert lazy_err == stack_err
        else:
            assert lazy_err == stack_err

        self.isValid = False
        self.capacity = 0
        self.allocated = 0
        self.epoch = 0
        self.lazy_allocations.clear()
        self.stack_allocations.clear()
        self.stack.clear()

    @invariant()
    def inv_same_allocation_count(self):
        assert len(self.lazy_allocations) == len(self.stack_allocations)

    @invariant()
    @precondition(lambda self: len(self.lazy_allocations) > 0)
    def inv_same_allocation_properties(self):
        assert all(
            lazy_size == stack_size
            and lazy_align == stack_align
            and lazy_epoch == stack_epoch
            for (_, lazy_size, lazy_align, lazy_epoch), (_, stack_size, stack_align, stack_epoch)
            in zip(self.lazy_allocations, self.stack_allocations)
        )

    @invariant()
    @precondition(lambda self: len(self.lazy_allocations) >= 2)
    def inv_same_memory_layout(self):
        lazy_base = ptr_to_int(self.lazy_allocations[0][0])
        stack_base = ptr_to_int(self.stack_allocations[0][0])

        assert all(
            (ptr_to_int(lazy_ptr) - lazy_base) == (ptr_to_int(stack_ptr) - stack_base)
            for (lazy_ptr, _, _, _), (stack_ptr, _, _, _) in zip(self.lazy_allocations, self.stack_allocations)
        )


TestDifferentialAllocatorModel = DifferentialAllocatorModel.TestCase
