#include "error/status.hpp"
#include "memory/scratch_allocator.hpp"
#include <cstddef>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py                      = pybind11;

constexpr const char* SCRATCH_TAG = "ScratchAllocator";
constexpr const char* MEM_TAG     = "memory";

void                  bind_scratch_allocator(pybind11::module_& module) { // NOLINT
        auto m  = module.def_submodule("scratch_allocator");

        m.doc() = "Anvil memory management library";

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

        m.def(
            "scratch_allocator_reset",
            [](py::capsule& allocator) {
                    anvil::memory::scratch_allocator::ScratchAllocator* alloc =
                        static_cast<anvil::memory::scratch_allocator::ScratchAllocator*>(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::scratch_allocator::ScratchAllocator*)0x1) {
                            anvil::memory::scratch_allocator::ScratchAllocator* null_alloc = nullptr;
                            return anvil::memory::scratch_allocator::reset(null_alloc);
                    }

                    return anvil::memory::scratch_allocator::reset(alloc);
            },
            py::arg("allocator"));

        m.def(
            "scratch_allocator_alloc",
            [](py::capsule& allocator, const size_t allocation_size, const size_t alignment) {
                    anvil::memory::scratch_allocator::ScratchAllocator* alloc =
                        static_cast<anvil::memory::scratch_allocator::ScratchAllocator*>(allocator.get_pointer());

                    char* allocation = nullptr;
                    if (alloc == nullptr || alloc == (anvil::memory::scratch_allocator::ScratchAllocator*)0x1) {
                            anvil::memory::scratch_allocator::ScratchAllocator* null_alloc = nullptr;

                            allocation =
                                static_cast<char*>(anvil::memory::scratch_allocator::alloc(null_alloc, allocation_size,
                                                                                                            alignment));

                            if (allocation == nullptr) {
                                    return py::make_tuple(py::none(), OK);
                            }
                    }

                    allocation =
                        static_cast<char*>(anvil::memory::scratch_allocator::alloc(alloc, allocation_size, alignment));

                    if (allocation == nullptr) {
                            return py::make_tuple(py::none(), OK);
                    }

                    return py::make_tuple(py::capsule(allocation, "char*"), OK);
            },
            py::arg("allocator"), py::arg("allocation_size"), py::arg("alignment"));
}
