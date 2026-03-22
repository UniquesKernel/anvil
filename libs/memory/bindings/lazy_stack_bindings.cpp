#include "error/status.hpp"
#include "memory/lazy_stack_allocator.hpp"
#include <cstring>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py = pybind11;

void bind_lazy_stack_allocator(pybind11::module_& module) { // NOLINT
        auto m  = module.def_submodule("lazy_stack_allocator");

        m.doc() = "Lazy Stack Allocator Module";

        m.def(
            "lazy_stack_allocator_create",
            [](const anvil::u64 capacity, const anvil::u64 alignment) -> py::tuple {
                    anvil::memory::LazyStackAllocator* allocator = nullptr;
                    const Error ERR = anvil::memory::create(&allocator, capacity, alignment);
                    if (ERR != OK) {
                            return py::make_tuple(ERR, py::none());
                    }
                    return py::make_tuple(ERR, py::capsule(allocator,
                                                           "anvil::memory::LazyStackAllocator"));
            },
            py::arg("capacity"), py::arg("alignment"));

        m.def(
            "lazy_stack_allocator_destroy",
            [](py::capsule& allocator) {
                    anvil::memory::LazyStackAllocator* alloc =
                        (anvil::memory::LazyStackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::LazyStackAllocator*)0x1) {
                            anvil::memory::LazyStackAllocator* null_alloc = nullptr;
                            return anvil::memory::destroy(&null_alloc);
                    }

                    const Error ERR = anvil::memory::destroy(&alloc);
                    if (ERR != OK) {
                            return ERR;
                    }

                    allocator.set_pointer((void*)(0x1));
                    return ERR;
            },
            py::arg("allocator"));

        m.def(
            "lazy_stack_allocator_reset",
            [](py::capsule& allocator) {
                    anvil::memory::LazyStackAllocator* alloc =
                        (anvil::memory::LazyStackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::LazyStackAllocator*)0x1) {
                            anvil::memory::LazyStackAllocator* null_alloc = nullptr;
                            return anvil::memory::reset(null_alloc);
                    }

                    return anvil::memory::reset(alloc);
            },
            py::arg("allocator"));

        m.def(
            "lazy_stack_allocator_alloc",
            [](py::capsule& allocator, const anvil::u64 allocation_size, const anvil::u64 alignment) -> py::tuple {
                    anvil::memory::LazyStackAllocator* alloc =
                        (anvil::memory::LazyStackAllocator*)(allocator.get_pointer());

                    char* allocation = nullptr;
                    if (alloc == nullptr || alloc == (anvil::memory::LazyStackAllocator*)0x1) {
                            anvil::memory::LazyStackAllocator* null_alloc = nullptr;

                            allocation = (char*)(anvil::memory::alloc(null_alloc, allocation_size,
                                                                                            alignment));

                            if (allocation == nullptr) {
                                    return py::make_tuple(py::none(), OK);
                            }
                            return py::make_tuple(py::capsule(allocation, "char*"), OK);
                    }

                    allocation = (char*)(anvil::memory::alloc(alloc, allocation_size, alignment));

                    if (allocation == nullptr) {
                            return py::make_tuple(py::none(), OK);
                    }

                    return py::make_tuple(py::capsule(allocation, "char*"), OK);
            },
            py::arg("allocator"), py::arg("allocation_size"), py::arg("alignment"));

        m.def(
            "lazy_stack_allocator_record",
            [](py::capsule& allocator) {
                    anvil::memory::LazyStackAllocator* alloc =
                        (anvil::memory::LazyStackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::LazyStackAllocator*)0x1) {
                            anvil::memory::LazyStackAllocator* null_alloc = nullptr;
                            return anvil::memory::record(null_alloc);
                    }

                    return anvil::memory::record(alloc);
            },
            py::arg("allocator"));

        m.def(
            "lazy_stack_allocator_unwind",
            [](py::capsule& allocator) {
                    anvil::memory::LazyStackAllocator* alloc =
                        (anvil::memory::LazyStackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::LazyStackAllocator*)0x1) {
                            anvil::memory::LazyStackAllocator* null_alloc = nullptr;
                            return anvil::memory::unwind(null_alloc);
                    }

                    return anvil::memory::unwind(alloc);
            },
            py::arg("allocator"));

        m.def(
            "lazy_stack_allocator_write",
            [](py::capsule& allocation, py::bytes data) -> Error {
                    char* ptr = (char*)(allocation.get_pointer());
                    if (ptr == nullptr || ptr == (char*)0x1) {
                            return NULL_PARAMETER;
                    }

                    std::string_view sv(data);
                    std::memcpy(ptr, sv.data(), sv.size());
                    return OK;
            },
            py::arg("allocation"), py::arg("data"));

        m.def(
            "lazy_stack_allocator_read",
            [](py::capsule& allocation, const anvil::u64 size) -> py::tuple {
                    char* ptr = (char*)(allocation.get_pointer());
                    if (ptr == nullptr || ptr == (char*)0x1) {
                            return py::make_tuple(NULL_PARAMETER, py::none());
                    }

                    return py::make_tuple(OK, py::bytes(ptr, size));
            },
            py::arg("allocation"), py::arg("size"));
}
