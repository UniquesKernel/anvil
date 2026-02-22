#include "error/status.hpp"
#include "memory/constants.hpp"
#include "memory/scratch_allocator.hpp"
#include <cstddef>
#include <pybind11/cast.h>
#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py                      = pybind11;

constexpr const char* SCRATCH_TAG = "ScratchAllocator";
constexpr const char* MEM_TAG     = "memory";

PYBIND11_MODULE(anvil_memory, m) {
        m.doc() = "Anvil memory management library";

        // Error codes
        py::enum_<Error>(m, "Error")
            .value("OK", OK)
            .value("INVALID_ARGUMENTS", INVALID_ARGUMENTS)
            .value("NULL_PARAMETER", NULL_PARAMETER)
            .export_values();

        // Constants
        m.attr("MIN_ALIGNMENT") = py::int_(anvil::memory::MIN_ALIGNMENT);
        m.attr("MAX_ALIGNMENT") = py::int_(anvil::memory::MAX_ALIGNMENT);
        m.attr("MAX_CAPACITY")  = py::int_(anvil::memory::MAX_CAPACITY);

        // ========== ScratchAllocator ==========
        m.def(
            "scratch_allocator_create",
            [](const size_t capacity, const size_t alignment) {
                    anvil::memory::scratch_allocator::ScratchAllocator* allocator = nullptr;
                    const Error ERR = anvil::memory::scratch_allocator::create(&allocator, capacity, alignment);
                    if (ERR != OK) {
                            return py::make_tuple(ERR, py::none());
                    }
                    return py::make_tuple(ERR,
                                          py::capsule(allocator, "anvil::memory::scratch_allocator::ScratchAllocator"));
            },
            py::arg("capacity"), py::arg("alignment"));

        m.def(
            "scratch_allocator_destroy",
            [](py::capsule& allocator) {
                    anvil::memory::scratch_allocator::ScratchAllocator* alloc =
                        static_cast<anvil::memory::scratch_allocator::ScratchAllocator*>(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::scratch_allocator::ScratchAllocator*)0x1) {
                            // NOTE: Rather than early return with the proper error code which would test the
                            // binding layer, we manufacture the same error code using C++ code thus exercising
                            // the proper path in the actual destroy function
                            anvil::memory::scratch_allocator::ScratchAllocator* null_alloc = nullptr;
                            return anvil::memory::scratch_allocator::destroy(&null_alloc);
                    }

                    const Error ERR = anvil::memory::scratch_allocator::destroy(&alloc);
                    if (ERR != OK) {
                            return ERR;
                    }

                    // NOTE: set_pointer does not accept nullptr, so for testing sake we use 0x1 as
                    // the nullptr
                    allocator.set_pointer(reinterpret_cast<void*>(0x1));
                    return ERR;
            },
            py::arg("allocator"));
}
