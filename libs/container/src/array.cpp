#include "anvil/container/array.hpp"
#include "anvil/error/assert.hpp"
#include "anvil/error/status.hpp"
#include "anvil/math/comparison/comparison.hpp"
#include "anvil/memory/resizeable_buffer.hpp"
#include "anvil/types.hpp"

namespace anvil::container {
Error create(Array* array, u64 type_size, u64 element_count) {
        REQUIRE(array != nullptr, NULL_PARAMETER);
        REQUIRE(math::comparison::is_power_of_two(type_size), INVALID_ARGUMENTS);

        const u64                                    CAP    = type_size * element_count;

        memory::resizeable_buffer::ResizeableBuffer* buffer = nullptr;
        (void)memory::resizeable_buffer::create(&buffer, CAP, alignof(void*));

        array->data      = buffer;
        array->size      = 0;
        array->capacity  = element_count;
        array->type_size = type_size;

        return anvil::OK;
}
} // namespace anvil::container
