#include "error/status.hpp"
#include "memory/stack_allocator.hpp"
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py = pybind11;

void bind_stack_allocator(pybind11::module_& module) { // NOLINT
        auto m  = module.def_submodule("stack_allocator");

        m.doc() = "Stack Allocator Module";

        m.def(
            "stack_allocator_create",
            [](const anvil::u64 capacity, const anvil::u64 alignment) -> py::tuple {
                    anvil::memory::stack_allocator::StackAllocator* allocator = nullptr;
                    const Error ERR = anvil::memory::stack_allocator::create(&allocator, capacity, alignment);
                    if (ERR != OK) {
                            return py::make_tuple(ERR, py::none());
                    }
                    return py::make_tuple(ERR,
                                          py::capsule(allocator, "anvil::memory::stack_allocator::StackAllocator"));
            },
            py::arg("capacity"), py::arg("alignment"));

        m.def(
            "stack_allocator_destroy",
            [](py::capsule& allocator) {
                    anvil::memory::stack_allocator::StackAllocator* alloc =
                        (anvil::memory::stack_allocator::StackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::stack_allocator::StackAllocator*)0x1) {
                            // NOTE: Rather than early return with the proper error code which would test the
                            // binding layer, we manufacture the same error code using C++ code thus exercising
                            // the proper path in the actual destroy function
                            anvil::memory::stack_allocator::StackAllocator* null_alloc = nullptr;
                            return anvil::memory::stack_allocator::destroy(&null_alloc);
                    }

                    const Error ERR = anvil::memory::stack_allocator::destroy(&alloc);
                    if (ERR != OK) {
                            return ERR;
                    }

                    // NOTE: set_pointer does not accept nullptr, so for testing sake we use 0x1 as
                    // the nullptr
                    allocator.set_pointer((void*)(0x1));
                    return ERR;
            },
            py::arg("allocator"));

        m.def(
            "stack_allocator_reset",
            [](py::capsule& allocator) {
                    anvil::memory::stack_allocator::StackAllocator* alloc =
                        (anvil::memory::stack_allocator::StackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::stack_allocator::StackAllocator*)0x1) {
                            anvil::memory::stack_allocator::StackAllocator* null_alloc = nullptr;
                            return anvil::memory::stack_allocator::reset(null_alloc);
                    }

                    return anvil::memory::stack_allocator::reset(alloc);
            },
            py::arg("allocator"));

        m.def(
            "stack_allocator_alloc",
            [](py::capsule& allocator, const anvil::u64 allocation_size, const anvil::u64 alignment) -> py::tuple {
                    anvil::memory::stack_allocator::StackAllocator* alloc =
                        (anvil::memory::stack_allocator::StackAllocator*)(allocator.get_pointer());

                    char* allocation = nullptr;
                    if (alloc == nullptr || alloc == (anvil::memory::stack_allocator::StackAllocator*)0x1) {
                            anvil::memory::stack_allocator::StackAllocator* null_alloc = nullptr;

                            allocation =
                                (char*)(anvil::memory::stack_allocator::alloc(null_alloc, allocation_size, alignment));

                            if (allocation == nullptr) {
                                    return py::make_tuple(py::none(), OK);
                            }
                            return py::make_tuple(py::capsule(allocation, "char*"), OK);
                    }

                    allocation = (char*)(anvil::memory::stack_allocator::alloc(alloc, allocation_size, alignment));

                    if (allocation == nullptr) {
                            return py::make_tuple(py::none(), OK);
                    }

                    return py::make_tuple(py::capsule(allocation, "char*"), OK);
            },
            py::arg("allocator"), py::arg("allocation_size"), py::arg("alignment"));

        m.def(
            "stack_allocator_record",
            [](py::capsule& allocator) {
                    anvil::memory::stack_allocator::StackAllocator* alloc =
                        (anvil::memory::stack_allocator::StackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::stack_allocator::StackAllocator*)0x1) {
                            // NOTE: Rather than early return with the proper error code which would test the
                            // binding layer, we manufacture the same error code using C++ code thus exercising
                            // the proper path in the actual record function
                            anvil::memory::stack_allocator::StackAllocator* null_alloc = nullptr;
                            return anvil::memory::stack_allocator::record(null_alloc);
                    }

                    return anvil::memory::stack_allocator::record(alloc);
            },
            py::arg("allocator"));

        m.def(
            "stack_allocator_unwind",
            [](py::capsule& allocator) {
                    anvil::memory::stack_allocator::StackAllocator* alloc =
                        (anvil::memory::stack_allocator::StackAllocator*)(allocator.get_pointer());

                    if (alloc == nullptr || alloc == (anvil::memory::stack_allocator::StackAllocator*)0x1) {
                            // NOTE: Rather than early return with the proper error code which would test the
                            // binding layer, we manufacture the same error code using C++ code thus exercising
                            // the proper path in the actual unwind function
                            anvil::memory::stack_allocator::StackAllocator* null_alloc = nullptr;
                            return anvil::memory::stack_allocator::unwind(null_alloc);
                    }

                    return anvil::memory::stack_allocator::unwind(alloc);
            },
            py::arg("allocator"));
}
