#include "memory/constants.hpp"
#include "memory/error.hpp"
#include "memory/scratch_allocator.hpp"
#include "memory/stack_allocator.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace {
constexpr const char* SCRATCH_TAG = "ScratchAllocator";
constexpr const char* STACK_TAG   = "StackAllocator";
constexpr const char* MEM_TAG     = "memory";

inline void* checked_ptr(const py::capsule& cap, const char* tag) {
    if (!cap) return nullptr;
    const char* name = cap.name();
    if (!name || std::strcmp(name, tag) != 0) {
        throw py::type_error(std::string("Invalid capsule tag; expected '") + tag + "'");
    }
    return cap.get_pointer(); // no-arg in pybind11
}

template <class T>
T* from_capsule(const py::capsule& cap, const char* tag) {
    return static_cast<T*>(checked_ptr(cap, tag));
}

inline py::object to_mem_capsule(void* p) {
    if (p) return py::capsule(p, MEM_TAG);
    return py::none();
}

inline int log2_exact(std::size_t v) {
    int e = 0; while ((std::size_t(1) << e) < v) ++e; return e;
}
} // namespace

PYBIND11_MODULE(anvil_memory, m) {
    m.doc() = "Anvil memory management library";

    // Error codes
    m.attr("ERR_SUCCESS")                  = py::int_(ERR_SUCCESS);
    m.attr("ERR_OUT_OF_MEMORY")            = py::int_(ERR_OUT_OF_MEMORY);
    m.attr("ERR_MEMORY_PERMISSION_CHANGE") = py::int_(ERR_MEMORY_PERMISSION_CHANGE);
    m.attr("ERR_MEMORY_DEALLOCATION")      = py::int_(ERR_MEMORY_DEALLOCATION);

    // Constants
    m.attr("EAGER") = py::int_(static_cast<std::size_t>(anvil::memory::AllocationStrategy::Eager));
    m.attr("LAZY")  = py::int_(static_cast<std::size_t>(anvil::memory::AllocationStrategy::Lazy));
    m.attr("MIN_ALIGNMENT") = py::int_(anvil::memory::MIN_ALIGNMENT);
    m.attr("MAX_ALIGNMENT") = py::int_(anvil::memory::MAX_ALIGNMENT);

    // Exponent ranges for testing (derived)
    m.attr("MIN_ALIGNMENT_EXPONENT") = py::int_(log2_exact(anvil::memory::MIN_ALIGNMENT));
    m.attr("MAX_ALIGNMENT_EXPONENT") = py::int_(log2_exact(anvil::memory::MAX_ALIGNMENT));

    // ========== ScratchAllocator ==========
    m.def("scratch_allocator_create",
          [](size_t capacity, size_t alignment) -> py::capsule {
              auto* a = anvil::memory::scratch_allocator::create(capacity, alignment);
              return a ? py::capsule(a, SCRATCH_TAG) : py::capsule();
          },
          py::arg("capacity"), py::arg("alignment"),
          "Create a scratch allocator");

    m.def("scratch_allocator_destroy",
          [](py::capsule cap) -> int {
              using SA = anvil::memory::scratch_allocator::ScratchAllocator;
              SA* a = from_capsule<SA>(cap, SCRATCH_TAG);
              if (!a) return -1;
              return static_cast<int>(anvil::memory::scratch_allocator::destroy(&a));
          },
          py::arg("allocator"), "Destroy a scratch allocator");

    m.def("scratch_allocator_alloc",
          [](py::capsule cap, size_t size, size_t alignment) -> py::object {
              using SA = anvil::memory::scratch_allocator::ScratchAllocator;
              SA* a = from_capsule<SA>(cap, SCRATCH_TAG);
              if (!a) return py::none();
              void* p = anvil::memory::scratch_allocator::alloc(a, size, alignment);
              return to_mem_capsule(p);
          },
          py::arg("allocator"), py::arg("size"), py::arg("alignment"),
          "Allocate memory from scratch allocator");

    m.def("scratch_allocator_reset",
          [](py::capsule cap) -> int {
              using SA = anvil::memory::scratch_allocator::ScratchAllocator;
              SA* a = from_capsule<SA>(cap, SCRATCH_TAG);
              if (!a) return -1;
              return static_cast<int>(anvil::memory::scratch_allocator::reset(a));
          },
          py::arg("allocator"), "Reset scratch allocator");

    // ========== StackAllocator ==========
    m.def("stack_allocator_create",
          [](size_t capacity, size_t alignment, size_t alloc_mode) -> py::capsule {
              auto mode = static_cast<anvil::memory::AllocationStrategy>(alloc_mode);
              auto* a = anvil::memory::stack_allocator::create(capacity, alignment, mode);
              return a ? py::capsule(a, STACK_TAG) : py::capsule();
          },
          py::arg("capacity"), py::arg("alignment"), py::arg("alloc_mode"),
          "Create a stack allocator");

    m.def("stack_allocator_destroy",
          [](py::capsule cap) -> int {
              using ST = anvil::memory::stack_allocator::StackAllocator;
              ST* a = from_capsule<ST>(cap, STACK_TAG);
              if (!a) return -1;
              return static_cast<int>(anvil::memory::stack_allocator::destroy(&a));
          },
          py::arg("allocator"), "Destroy a stack allocator");

    m.def("stack_allocator_alloc",
          [](py::capsule cap, size_t size, size_t alignment) -> py::object {
              using ST = anvil::memory::stack_allocator::StackAllocator;
              ST* a = from_capsule<ST>(cap, STACK_TAG);
              if (!a) return py::none();
              void* p = anvil::memory::stack_allocator::alloc(a, size, alignment);
              return to_mem_capsule(p);
          },
          py::arg("allocator"), py::arg("size"), py::arg("alignment"),
          "Allocate memory from stack allocator");

    m.def("stack_allocator_reset",
          [](py::capsule cap) -> int {
              using ST = anvil::memory::stack_allocator::StackAllocator;
              ST* a = from_capsule<ST>(cap, STACK_TAG);
              if (!a) return -1;
              return static_cast<int>(anvil::memory::stack_allocator::reset(a));
          },
          py::arg("allocator"), "Reset stack allocator");

    m.def("stack_allocator_record",
          [](py::capsule cap) -> int {
              using ST = anvil::memory::stack_allocator::StackAllocator;
              ST* a = from_capsule<ST>(cap, STACK_TAG);
              if (!a) return -1;
              return static_cast<int>(anvil::memory::stack_allocator::record(a));
          },
          py::arg("allocator"), "Record current allocation state");

    m.def("stack_allocator_unwind",
          [](py::capsule cap) -> int {
              using ST = anvil::memory::stack_allocator::StackAllocator;
              ST* a = from_capsule<ST>(cap, STACK_TAG);
              if (!a) return -1;
              return static_cast<int>(anvil::memory::stack_allocator::unwind(a));
          },
          py::arg("allocator"), "Unwind to last recorded state");

    // ========== Helpers ==========
    m.def("read_bytes",
          [](py::capsule cap, size_t size) -> py::bytes {
              const void* p = checked_ptr(cap, MEM_TAG);
              if (!p) return py::bytes();
              return py::bytes(static_cast<const char*>(p), size);
          },
          py::arg("ptr"), py::arg("size"),
          "Read bytes from a memory address");

    m.def("ptr_to_int",
          [](py::capsule cap) -> uintptr_t {
              void* p = checked_ptr(cap, MEM_TAG);
              return reinterpret_cast<uintptr_t>(p);
          },
          py::arg("ptr"), "Convert a pointer capsule to integer address");

    m.def("write_bytes",
          [](py::capsule cap, py::bytes data) {
              void* p = checked_ptr(cap, MEM_TAG);
              if (!p) return;
              const std::string buf = data; // py::bytes -> std::string
              std::memcpy(p, buf.data(), buf.size());
          },
          py::arg("ptr"), py::arg("data"),
          "Write bytes to a memory address");
}
