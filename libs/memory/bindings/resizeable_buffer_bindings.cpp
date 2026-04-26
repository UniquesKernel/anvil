#include "anvil/error/status.hpp"
#include <anvil/memory/resizeable_buffer.hpp>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py = pybind11;

void bind_resizeable_buffer(py::module_& module) {
        auto m  = module.def_submodule("resizeable_buffer");

        m.doc() = "Anvil memory management library";

        m.def(
            "resizeable_buffer_create",
            [](const anvil::u64 capacity, const anvil::u64 alignment) -> py::tuple {
                    anvil::memory::resizeable_buffer::ResizeableBuffer* buffer = nullptr;
                    const anvil::Error ERR = anvil::memory::resizeable_buffer::create(&buffer, capacity, alignment);
                    if (ERR != anvil::OK) {
                            return py::make_tuple(ERR, py::none());
                    }
                    return py::make_tuple(ERR, py::capsule(buffer, "anvil::memory::ResizeableBuffer"));
            },
            py::arg("capacity"), py::arg("alignment"));
}
