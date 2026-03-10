#include "error/status.hpp"
#include "memory/lazy_stack_allocator.hpp"
#include <cstddef>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py = pybind11;

void bind_lazy_stack_allocator(pybind11::module_& module) { // NOLINT
        auto m  = module.def_submodule("lazy_stack_allocator");

        m.doc() = "Lazy Stack Allocator Module";

        m.def(
            "lazy_stack_allocator_create",
            [](const size_t capacity, const size_t alignment) -> py::tuple {
                    anvil::memory::lazy_stack_allocator::LazyStackAllocator* allocator = nullptr;
                    const Error ERR = anvil::memory::lazy_stack_allocator::create(&allocator, capacity, alignment);
                    if (ERR != OK) {
                            return py::make_tuple(ERR, py::none());
                    }
                    return py::make_tuple(ERR, py::capsule(allocator,
                                                           "anvil::memory::lazy_stack_allocator::LazyStackAllocator"));
            },
            py::arg("capacity"), py::arg("alignment"));

        m.def(
            "lazy_stack_allocator_destroy",
            [](py::capsule& allocator) {
                                        anvil::memory::lazy_stack_allocator::LazyStackAllocator* alloc =
                                                (anvil::memory::lazy_stack_allocator::LazyStackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::lazy_stack_allocator::LazyStackAllocator*)0x1) {
                            anvil::memory::lazy_stack_allocator::LazyStackAllocator* null_alloc = nullptr;
                            return anvil::memory::lazy_stack_allocator::destroy(&null_alloc);
                    }

                    const Error ERR = anvil::memory::lazy_stack_allocator::destroy(&alloc);
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
                                        anvil::memory::lazy_stack_allocator::LazyStackAllocator* alloc =
                                                (anvil::memory::lazy_stack_allocator::LazyStackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::lazy_stack_allocator::LazyStackAllocator*)0x1) {
                            anvil::memory::lazy_stack_allocator::LazyStackAllocator* null_alloc = nullptr;
                            return anvil::memory::lazy_stack_allocator::reset(null_alloc);
                    }

                    return anvil::memory::lazy_stack_allocator::reset(alloc);
            },
            py::arg("allocator"));

        m.def(
            "lazy_stack_allocator_alloc",
            [](py::capsule& allocator, const size_t allocation_size, const size_t alignment) -> py::tuple {
                                        anvil::memory::lazy_stack_allocator::LazyStackAllocator* alloc =
                                                (anvil::memory::lazy_stack_allocator::LazyStackAllocator*)(allocator.get_pointer());

                    char* allocation = nullptr;
                    if (alloc == nullptr || alloc == (anvil::memory::lazy_stack_allocator::LazyStackAllocator*)0x1) {
                            anvil::memory::lazy_stack_allocator::LazyStackAllocator* null_alloc = nullptr;

                                                        allocation =
                                                                (char*)(anvil::memory::lazy_stack_allocator::alloc(null_alloc, allocation_size, alignment));

                            if (allocation == nullptr) {
                                    return py::make_tuple(py::none(), OK);
                            }
                            return py::make_tuple(py::capsule(allocation, "char*"), OK);
                    }

                    allocation = (char*)(anvil::memory::lazy_stack_allocator::alloc(alloc, allocation_size, alignment));

                    if (allocation == nullptr) {
                            return py::make_tuple(py::none(), OK);
                    }

                    return py::make_tuple(py::capsule(allocation, "char*"), OK);
            },
            py::arg("allocator"), py::arg("allocation_size"), py::arg("alignment"));

        m.def(
            "lazy_stack_allocator_record",
            [](py::capsule& allocator) {
                                        anvil::memory::lazy_stack_allocator::LazyStackAllocator* alloc =
                                                (anvil::memory::lazy_stack_allocator::LazyStackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::lazy_stack_allocator::LazyStackAllocator*)0x1) {
                            anvil::memory::lazy_stack_allocator::LazyStackAllocator* null_alloc = nullptr;
                            return anvil::memory::lazy_stack_allocator::record(null_alloc);
                    }

                    return anvil::memory::lazy_stack_allocator::record(alloc);
            },
            py::arg("allocator"));

        m.def(
            "lazy_stack_allocator_unwind",
            [](py::capsule& allocator) {
                                        anvil::memory::lazy_stack_allocator::LazyStackAllocator* alloc =
                                                (anvil::memory::lazy_stack_allocator::LazyStackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::lazy_stack_allocator::LazyStackAllocator*)0x1) {
                            anvil::memory::lazy_stack_allocator::LazyStackAllocator* null_alloc = nullptr;
                            return anvil::memory::lazy_stack_allocator::unwind(null_alloc);
                    }

                    return anvil::memory::lazy_stack_allocator::unwind(alloc);
            },
            py::arg("allocator"));
}
