from anvil_memory.resizeable_buffer import (
    resizeable_buffer_create,
    resizeable_buffer_destroy,
    resizeable_buffer_resize,
    ResizeableBuffer,
    resizeable_buffer_write,
)
from anvil_memory import Error
from hypothesis.stateful import RuleBasedStateMachine, rule, initialize, precondition
from hypothesis import strategies as st


@st.composite
def buffer_config(draw):
    """Generate (capacity, alignment) where capacity >= alignment"""
    base = draw(st.integers(min_value=3, max_value=11))
    alignment = 2**base

    capacity = draw(st.integers(min_value=alignment, max_value=2**12))
    return (capacity, alignment)


class ResizeableBufferModel(RuleBasedStateMachine):
    def __init__(self) -> None:
        super().__init__()
        self.buffer: ResizeableBuffer = None
        self.isValid = False

    @initialize(config=buffer_config())
    def intialize(self, config):
        capacity, alignment = config
        err, buffer = resizeable_buffer_create(capacity, alignment)

        if err != Error.OK:
            self.isValid = False
            return

        assert err == Error.OK
        assert buffer is not None

        self.buffer = buffer
        self.isValid = True

    @rule(config=buffer_config())
    @precondition(lambda self: self.isValid is False)
    def create(self, config):
        capacity, alignment = config
        err, buffer = resizeable_buffer_create(capacity, alignment)

        if err != Error.OK:
            self.isValid = False
            return

        assert err == Error.OK
        assert buffer is not None

        self.buffer = buffer
        self.isValid = True

    @rule()
    @precondition(lambda self: self.isValid is True)
    def destroy(self):
        err = resizeable_buffer_destroy(self.buffer)
        if self.isValid == True:
            assert err == Error.OK
            self.isValid = False
        else:
            assert err == error.NULL_PARAMETER

    @rule(resize=st.integers(min_value=0, max_value=10000))
    @precondition(lambda self: self.isValid is True)
    def resize(self, resize):
        pass

    @rule(data=st.data())
    @precondition(lambda self: self.isValid is True)
    def write_to_allocation(self, data):
        payload_size = data.draw(
            st.integers(min_value=0, max_value=self.buffer.capacity - 1)
        )
        payload = bytes(i & 0xFF for i in range(payload_size))
        err = resizeable_buffer_write(self.buffer, payload)
        assert err == Error.OK


#       err, result = scratch_allocator_read(ptr, allocation_size)
#      assert err == Error.OK
#      assert result == payload, "Write/read mismatch"


TestResizeableBufferModel = ResizeableBufferModel.TestCase
