#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "memory/error.hpp"
#include "memory/constants.hpp"
#include "memory/scratch_allocator.hpp"
#include "memory/stack_allocator.hpp"
#include "memory/pool_allocator.hpp"
#include <cstring>

namespace py = pybind11;

PYBIND11_MODULE(anvil_memory, m) {
    m.doc() = "Anvil memory management library";

    // Error codes
    m.attr("ERR_SUCCESS") = static_cast<int>(ERR_SUCCESS);
    m.attr("ERR_OUT_OF_MEMORY") = static_cast<int>(ERR_OUT_OF_MEMORY);
    m.attr("ERR_MEMORY_PERMISSION_CHANGE") = static_cast<int>(ERR_MEMORY_PERMISSION_CHANGE);
    m.attr("ERR_MEMORY_DEALLOCATION") = static_cast<int>(ERR_MEMORY_DEALLOCATION);
    m.attr("ERR_MEMORY_WRITE_ERROR") = static_cast<int>(ERR_MEMORY_WRITE_ERROR);
    
    // Constants
    m.attr("EAGER") = static_cast<int>(EAGER);
    m.attr("LAZY") = static_cast<int>(LAZY);
    m.attr("MIN_ALIGNMENT") = static_cast<int>(MIN_ALIGNMENT);
    m.attr("MAX_ALIGNMENT") = static_cast<int>(MAX_ALIGNMENT);

    // Exponent ranges for testing (since alignment = 1 << exponent)
    m.attr("MIN_ALIGNMENT_EXPONENT") = 0;  // 1 << 0 = 1
    m.attr("MAX_ALIGNMENT_EXPONENT") = 11; // 1 << 11 = 2048

    // Error domains
    py::enum_<ErrorDomain>(m, "ErrorDomain")
        .value("NONE", ERR_DOMAIN_NONE)
        .value("MEMORY", ERR_DOMAIN_MEMORY)
        .value("IO", ERR_DOMAIN_IO)
        .value("NETWORK", ERR_DOMAIN_NETWORK)
        .value("STATE", ERR_DOMAIN_STATE)
        .value("VALUE", ERR_DOMAIN_VALUE)
        .export_values();

    py::enum_<ErrorSeverity>(m, "ErrorSeverity")
        .value("INFO", ERR_SEVERITY_INFO)
        .value("WARNING", ERR_SEVERITY_WARNING)
        .value("ERROR", ERR_SEVERITY_ERROR)
        .value("FATAL", ERR_SEVERITY_FATAL)
        .export_values();

    // ========== ScratchAllocator Functions ==========
    m.def("scratch_allocator_create",
        [](size_t capacity, size_t alignment) -> py::capsule {
            ScratchAllocator* alloc = anvil::memory::scratch_allocator::create(capacity, alignment);
            if (!alloc) return py::capsule();
            return py::capsule(alloc, "ScratchAllocator");
        },
        py::arg("capacity"), py::arg("alignment"),
        "Create a scratch allocator");
    
    m.def("scratch_allocator_destroy",
        [](py::capsule cap) -> int {
            ScratchAllocator* alloc = static_cast<ScratchAllocator*>(cap);
            if (!alloc) return -1;
            return static_cast<int>(anvil::memory::scratch_allocator::destroy(&alloc));
        },
        py::arg("allocator"),
        "Destroy a scratch allocator");
    
    m.def("scratch_allocator_alloc",
        [](py::capsule allocator_cap, size_t size, size_t alignment) -> py::object {
            ScratchAllocator* alloc = static_cast<ScratchAllocator*>(allocator_cap);
            if (!alloc) return py::none();
            
            void* ptr = anvil::memory::scratch_allocator::alloc(alloc, size, alignment);
            if (!ptr) return py::none();
            return py::capsule(ptr, "memory");
        },
        py::arg("allocator"), py::arg("size"), py::arg("alignment"),
        "Allocate memory from scratch allocator");
    
    m.def("scratch_allocator_reset",
        [](py::capsule cap) -> int {
            ScratchAllocator* alloc = static_cast<ScratchAllocator*>(cap);
            if (!alloc) return -1;
            return static_cast<int>(anvil::memory::scratch_allocator::reset(alloc));
        },
        py::arg("allocator"),
        "Reset scratch allocator");
    
    m.def("scratch_allocator_copy",
        [](py::capsule allocator_cap, py::bytes data) -> py::object {
            ScratchAllocator* alloc = static_cast<ScratchAllocator*>(allocator_cap);
            if (!alloc) return py::none();
            
            char* buffer = nullptr;
            ssize_t length = 0;
            if (PYBIND11_BYTES_AS_STRING_AND_SIZE(data.ptr(), &buffer, &length) == -1) {
                return py::none();
            }
            
            void* ptr = anvil::memory::scratch_allocator::copy(alloc, buffer, length);
            if (!ptr) return py::none();
            return py::capsule(ptr, "memory");
        },
        py::arg("allocator"), py::arg("data"),
        "Copy data into scratch allocator");

    // ========== StackAllocator Functions ==========
    m.def("stack_allocator_create",
        [](size_t capacity, size_t alignment, size_t alloc_mode) -> py::capsule {
            StackAllocator* alloc = anvil::memory::stack_allocator::create(capacity, alignment, alloc_mode);
            if (!alloc) return py::capsule();
            return py::capsule(alloc, "StackAllocator");
        },
        py::arg("capacity"), py::arg("alignment"), py::arg("alloc_mode"),
        "Create a stack allocator");
    
    m.def("stack_allocator_destroy",
        [](py::capsule cap) -> int {
            StackAllocator* alloc = static_cast<StackAllocator*>(cap);
            if (!alloc) return -1;
            return static_cast<int>(anvil::memory::stack_allocator::destroy(&alloc));
        },
        py::arg("allocator"),
        "Destroy a stack allocator");
    
    m.def("stack_allocator_alloc",
        [](py::capsule allocator_cap, size_t size, size_t alignment) -> py::object {
            StackAllocator* alloc = static_cast<StackAllocator*>(allocator_cap);
            if (!alloc) return py::none();
            
            void* ptr = anvil::memory::stack_allocator::alloc(alloc, size, alignment);
            if (!ptr) return py::none();
            return py::capsule(ptr, "memory");
        },
        py::arg("allocator"), py::arg("size"), py::arg("alignment"),
        "Allocate memory from stack allocator");
    
    m.def("stack_allocator_reset",
        [](py::capsule cap) -> int {
            StackAllocator* alloc = static_cast<StackAllocator*>(cap);
            if (!alloc) return -1;
            return static_cast<int>(anvil::memory::stack_allocator::reset(alloc));
        },
        py::arg("allocator"),
        "Reset stack allocator");
    
    m.def("stack_allocator_copy",
        [](py::capsule allocator_cap, py::bytes data) -> py::object {
            StackAllocator* alloc = static_cast<StackAllocator*>(allocator_cap);
            if (!alloc) return py::none();
            
            char* buffer = nullptr;
            ssize_t length = 0;
            if (PYBIND11_BYTES_AS_STRING_AND_SIZE(data.ptr(), &buffer, &length) == -1) {
                return py::none();
            }
            
            void* ptr = anvil::memory::stack_allocator::copy(alloc, buffer, length);
            if (!ptr) return py::none();
            return py::capsule(ptr, "memory");
        },
        py::arg("allocator"), py::arg("data"),
        "Copy data into stack allocator");
    
    m.def("stack_allocator_record",
        [](py::capsule cap) -> int {
            StackAllocator* alloc = static_cast<StackAllocator*>(cap);
            if (!alloc) return -1;
            return static_cast<int>(anvil::memory::stack_allocator::record(alloc));
        },
        py::arg("allocator"),
        "Record current allocation state");
    
    m.def("stack_allocator_unwind",
        [](py::capsule cap) -> int {
            StackAllocator* alloc = static_cast<StackAllocator*>(cap);
            if (!alloc) return -1;
            return static_cast<int>(anvil::memory::stack_allocator::unwind(alloc));
        },
        py::arg("allocator"),
        "Unwind to last recorded state");

    // ========== Helper Functions ==========
    m.def("read_bytes",
        [](py::capsule cap, size_t size) -> py::bytes {
            void* ptr = static_cast<void*>(cap);
            if (!ptr) return py::bytes();
            return py::bytes(static_cast<const char*>(ptr), size);
        },
        py::arg("ptr"), py::arg("size"),
        "Read bytes from a memory address");
    
    m.def("ptr_to_int",
        [](py::capsule cap) -> uintptr_t {
            void* ptr = static_cast<void*>(cap);
            return reinterpret_cast<uintptr_t>(ptr);
        },
        py::arg("ptr"),
        "Convert a pointer capsule to integer address");
    
    m.def("write_bytes",
        [](py::capsule cap, py::bytes data) {
            void* ptr = static_cast<void*>(cap);
            if (!ptr) return;
            
            char* buffer = nullptr;
            ssize_t length = 0;
            if (PYBIND11_BYTES_AS_STRING_AND_SIZE(data.ptr(), &buffer, &length) == -1) {
                return;
            }
            
            std::memcpy(ptr, buffer, length);
        },
        py::arg("ptr"), py::arg("data"),
        "Write bytes to a memory address");
}