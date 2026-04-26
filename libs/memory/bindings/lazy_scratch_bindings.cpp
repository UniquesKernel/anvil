#include "anvil/error/status.hpp"
#include "anvil/memory/lazy_scratch_allocator.hpp"
#include <cstring>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py = pybind11;

void bind_lazy_scratch_allocator(pybind11::module_& module) { // NOLINT
        auto m  = module.def_submodule("lazy_scratch_allocator");

        m.doc() = "Lazy Scratch Allocator Module";

        m.def(
            "lazy_scratch_allocator_create",
            [](const anvil::u64 capacity, const anvil::u64 alignment) -> py::tuple {
                    anvil::memory::LazyScratchAllocator* allocator = nullptr;
                    const anvil::Error ERR = anvil::memory::create(&allocator, capacity, alignment);
                    if (ERR != anvil::OK) {
                            return py::make_tuple(ERR, py::none());
                    }
                    return py::make_tuple(ERR,
                                          py::capsule(allocator,
                                                      "anvil::memory::LazyScratchAllocator"));
            },
            py::arg("capacity"), py::arg("alignment"));

        m.def(
            "lazy_scratch_allocator_destroy",
            [](py::capsule& allocator) {
                    anvil::memory::LazyScratchAllocator* alloc =
                        (anvil::memory::LazyScratchAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr ||
                        alloc == (anvil::memory::LazyScratchAllocator*)0x1) {
                            anvil::memory::LazyScratchAllocator* null_alloc = nullptr;
                            return anvil::memory::destroy(&null_alloc);
                    }

                    const anvil::Error ERR = anvil::memory::destroy(&alloc);
                    if (ERR != anvil::OK) {
                            return ERR;
                    }

                    allocator.set_pointer((void*)(0x1));
                    return ERR;
            },
            py::arg("allocator"));

        m.def(
            "lazy_scratch_allocator_reset",
            [](py::capsule& allocator) {
                    anvil::memory::LazyScratchAllocator* alloc =
                        (anvil::memory::LazyScratchAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr ||
                        alloc == (anvil::memory::LazyScratchAllocator*)0x1) {
                            anvil::memory::LazyScratchAllocator* null_alloc = nullptr;
                            return anvil::memory::reset(null_alloc);
                    }

                    return anvil::memory::reset(alloc);
            },
            py::arg("allocator"));

        m.def(
            "lazy_scratch_allocator_alloc",
            [](py::capsule& allocator, const anvil::u64 allocation_size, const anvil::u64 alignment) -> py::tuple {
                    anvil::memory::LazyScratchAllocator* alloc =
                        (anvil::memory::LazyScratchAllocator*)(allocator.get_pointer());

                    char* allocation = nullptr;
                    if (alloc == nullptr ||
                        alloc == (anvil::memory::LazyScratchAllocator*)0x1) {
                            anvil::memory::LazyScratchAllocator* null_alloc = nullptr;

                            allocation =
                                (char*)(anvil::memory::alloc(null_alloc, allocation_size,
                                                                                     alignment));

                            if (allocation == nullptr) {
                                    return py::make_tuple(py::none(), anvil::OK);
                            }
                            return py::make_tuple(py::capsule(allocation, "char*"), anvil::OK);
                    }

                    allocation =
                        (char*)(anvil::memory::alloc(alloc, allocation_size, alignment));

                    if (allocation == nullptr) {
                            return py::make_tuple(py::none(), anvil::OK);
                    }

                    return py::make_tuple(py::capsule(allocation, "char*"), anvil::OK);
            },
            py::arg("allocator"), py::arg("allocation_size"), py::arg("alignment"));

        m.def(
            "lazy_scratch_allocator_write",
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
            "lazy_scratch_allocator_read",
            [](py::capsule& allocation, const anvil::u64 size) -> py::tuple {
                    char* ptr = (char*)(allocation.get_pointer());
                    if (ptr == nullptr || ptr == (char*)0x1) {
                            return py::make_tuple(anvil::NULL_PARAMETER, py::none());
                    }

                    return py::make_tuple(anvil::OK, py::bytes(ptr, size));
            },
            py::arg("allocation"), py::arg("size"));
}
