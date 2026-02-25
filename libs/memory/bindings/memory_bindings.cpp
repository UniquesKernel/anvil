#include "error/status.hpp"
#include "memory/constants.hpp"
#include <cstdint>
#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

void bind_scratch_allocator(pybind11::module_& module);
void bind_stack_allocator(pybind11::module_& module);

namespace py = pybind11;

PYBIND11_MODULE(anvil_memory, m) {
        m.doc() = "Anvil Memory Library";

        bind_scratch_allocator(m);
        bind_stack_allocator(m);

        // Error codes
        py::enum_<Error>(m, "Error")
            .value("OK", OK)
            .value("INVALID_ARGUMENTS", INVALID_ARGUMENTS)
            .value("NULL_PARAMETER", NULL_PARAMETER)
            .value("OUT_OF_MEMORY", OUT_OF_MEMORY)
            .export_values();

        // Constants
        m.attr("MIN_ALIGNMENT")   = py::int_(anvil::memory::MIN_ALIGNMENT);
        m.attr("MAX_ALIGNMENT")   = py::int_(anvil::memory::MAX_ALIGNMENT);
        m.attr("MAX_CAPACITY")    = py::int_(anvil::memory::MAX_CAPACITY);
        m.attr("MAX_STACK_DEPTH") = py::int_(anvil::memory::MAX_STACK_DEPTH);

        m.def(
            "ptr_to_int", [](py::capsule& cap) { return reinterpret_cast<uintptr_t>(cap.get_pointer()); },
            py::arg("ptr"), "Convert a pointer capsule to integer address");
}
