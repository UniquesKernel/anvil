#include "anvil/error/status.hpp"
#include <anvil/memory/resizeable_buffer.hpp>
#include <pybind11/cast.h>
#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py = pybind11;

void bind_resizeable_buffer(py::module_& module) {
        auto m  = module.def_submodule("resizeable_buffer");

        m.doc() = "Anvil memory management library";

        py::class_<anvil::memory::resizeable_buffer::ResizeableBuffer>(m, "ResizeableBuffer")
            .def(py::init<>())
            .def_readonly("capacity", &anvil::memory::resizeable_buffer::ResizeableBuffer::capacity)
            .def_readonly("base", &anvil::memory::resizeable_buffer::ResizeableBuffer::base);

        m.def(
            "resizeable_buffer_create",
            [](const anvil::u64 capacity, const anvil::u64 alignment) -> py::tuple {
                    anvil::memory::resizeable_buffer::ResizeableBuffer* buffer = nullptr;
                    const anvil::Error ERR = anvil::memory::resizeable_buffer::create(&buffer, capacity, alignment);
                    if (ERR != anvil::OK) {
                            return py::make_tuple(ERR, py::none());
                    }
                    return py::make_tuple(ERR, py::cast(buffer, py::return_value_policy::reference));
            },
            py::arg("capacity"), py::arg("alignment"));

        m.def(
            "resizeable_buffer_destroy",
            [](anvil::memory::resizeable_buffer::ResizeableBuffer* buffer) {
                    if (buffer == nullptr) {
                            // NOTE: Rather than early return with the proper error code which would test the
                            // binding layer, we manufacture the same error code using C++ code thus exercising
                            // the proper path in the actual destroy function
                            anvil::memory::resizeable_buffer::ResizeableBuffer* null_buf = nullptr;
                            return anvil::memory::resizeable_buffer::destroy(&null_buf);
                    }

                    const anvil::Error ERR = anvil::memory::resizeable_buffer::destroy(&buffer);
                    if (ERR != anvil::OK) {
                            return ERR;
                    }

                    return ERR;
            },
            py::arg("buffer"));

        m.def(
            "resizeable_buffer_resize",
            [](anvil::memory::resizeable_buffer::ResizeableBuffer* buffer, const anvil::u64 new_capacity) {
                    return anvil::memory::resizeable_buffer::resize(&buffer, new_capacity);
            },
            py::arg("buffer"), py::arg("new_capacity"));

        m.def(
            "resizeable_buffer_write",
            [](anvil::memory::resizeable_buffer::ResizeableBuffer* buffer, py::bytes data) -> anvil::Error {
                    if (buffer->base == nullptr || buffer->base == (char*)0x1) {
                            return anvil::NULL_PARAMETER;
                    }

                    std::string_view sv(data);
                    std::memcpy(buffer->base, sv.data(), sv.size());
                    return anvil::OK;
            },
            py::arg("buffer"), py::arg("data"));
}
