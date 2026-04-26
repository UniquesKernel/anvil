#include "anvil/error/status.hpp"
#include "anvil/memory/scratch_allocator.hpp"
#include <cstring>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py                      = pybind11;

constexpr const char* SCRATCH_TAG = "ScratchAllocator";
constexpr const char* MEM_TAG     = "memory";

void                  bind_scratch_allocator(pybind11::module_& module) { // NOLINT
        auto m  = module.def_submodule("scratch_allocator");

        m.doc() = "Anvil memory management library";

        m.def(
            "scratch_allocator_create",
            [](const anvil::u64 capacity, const anvil::u64 alignment) -> py::tuple {
                    anvil::memory::ScratchAllocator* allocator = nullptr;
                    const anvil::Error               ERR = anvil::memory::create(&allocator, capacity, alignment);
                    if (ERR != anvil::OK) {
                            return py::make_tuple(ERR, py::none());
                    }
                    return py::make_tuple(ERR, py::capsule(allocator, "anvil::memory::ScratchAllocator"));
            },
            py::arg("capacity"), py::arg("alignment"));

        m.def(
            "scratch_allocator_destroy",
            [](py::capsule& allocator) {
                    anvil::memory::ScratchAllocator* alloc =
                        (anvil::memory::ScratchAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::ScratchAllocator*)0x1) {
                            // NOTE: Rather than early return with the proper error code which would test the
                            // binding layer, we manufacture the same error code using C++ code thus exercising
                            // the proper path in the actual destroy function
                            anvil::memory::ScratchAllocator* null_alloc = nullptr;
                            return anvil::memory::destroy(&null_alloc);
                    }

                    const anvil::Error ERR = anvil::memory::destroy(&alloc);
                    if (ERR != anvil::OK) {
                            return ERR;
                    }

                    // NOTE: set_pointer does not accept nullptr, so for testing sake we use 0x1 as
                    // the nullptr
                    allocator.set_pointer((void*)(0x1));
                    return ERR;
            },
            py::arg("allocator"));

        m.def(
            "scratch_allocator_reset",
            [](py::capsule& allocator) {
                    anvil::memory::ScratchAllocator* alloc =
                        (anvil::memory::ScratchAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::ScratchAllocator*)0x1) {
                            anvil::memory::ScratchAllocator* null_alloc = nullptr;
                            return anvil::memory::reset(null_alloc);
                    }

                    return anvil::memory::reset(alloc);
            },
            py::arg("allocator"));

        m.def(
            "scratch_allocator_alloc",
            [](py::capsule& allocator, const anvil::u64 allocation_size, const anvil::u64 alignment) -> py::tuple {
                    anvil::memory::ScratchAllocator* alloc =
                        (anvil::memory::ScratchAllocator*)(allocator.get_pointer());

                    char* allocation = nullptr;
                    if (alloc == nullptr || alloc == (anvil::memory::ScratchAllocator*)0x1) {
                            anvil::memory::ScratchAllocator* null_alloc = nullptr;

                            allocation = (char*)(anvil::memory::alloc(null_alloc, allocation_size, alignment));

                            if (allocation == nullptr) {
                                    return py::make_tuple(py::none(), anvil::OK);
                            }
                            return py::make_tuple(py::capsule(allocation, "char*"), anvil::OK);
                    }

                    allocation = (char*)(anvil::memory::alloc(alloc, allocation_size, alignment));

                    if (allocation == nullptr) {
                            return py::make_tuple(py::none(), anvil::OK);
                    }

                    return py::make_tuple(py::capsule(allocation, "char*"), anvil::OK);
            },
            py::arg("allocator"), py::arg("allocation_size"), py::arg("alignment"));

        m.def(
            "scratch_allocator_write",
            [](py::capsule& allocation, py::bytes data) -> anvil::Error {
                    char* ptr = (char*)(allocation.get_pointer());
                    if (ptr == nullptr || ptr == (char*)0x1) {
                            return anvil::NULL_PARAMETER;
                    }

                    std::string_view sv(data);
                    std::memcpy(ptr, sv.data(), sv.size());
                    return anvil::OK;
            },
            py::arg("allocation"), py::arg("data"));

        m.def(
            "scratch_allocator_read",
            [](py::capsule& allocation, const anvil::u64 size) -> py::tuple {
                    char* ptr = (char*)(allocation.get_pointer());
                    if (ptr == nullptr || ptr == (char*)0x1) {
                            return py::make_tuple(anvil::NULL_PARAMETER, py::none());
                    }

                    return py::make_tuple(anvil::OK, py::bytes(ptr, size));
            },
            py::arg("allocation"), py::arg("size"));
}
