#include "math/vector.hpp"
#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(math_module, module) {
        module.doc() = "Vector module";
        py::class_<anvil::math::Vector2>(module, "Vector2")
            .def(py::init([](const float& a, const float& b) { return anvil::math::Vector2{a, b}; }))
            .def_property(
                "x", [](anvil::math::Vector2& v) { return pybind11::module_::import("numpy").attr("float32")(v.x); },
                [](anvil::math::Vector2& v, float val) { v.x = val; })
            .def_property(
                "y", [](anvil::math::Vector2& v) { return pybind11::module_::import("numpy").attr("float32")(v.y); },
                [](anvil::math::Vector2& v, float val) { v.y = val; })
            .def("__add__", [](const anvil::math::Vector2& A, const anvil::math::Vector2& B) { return A + B; })
            .def("__sub__", [](const anvil::math::Vector2& A, const anvil::math::Vector2& B) { return A - B; })
            .def("__neg__", [](const anvil::math::Vector2& A) { return -A; })
            .def("__mul__",
                 [](const anvil::math::Vector2& A, const anvil::math::Vector2& B) {
                         return pybind11::module_::import("numpy").attr("float32")(A * B);
                 })
            .def("__mul__", [](const anvil::math::Vector2& A, const float SCALAR) { return A * SCALAR; })
            .def("__rmul__", [](const anvil::math::Vector2& A, const float SCALAR) { return SCALAR * A; })
            .def("__eq__", [](const anvil::math::Vector2& A, const anvil::math::Vector2& B) { return A == B; });

        py::class_<anvil::math::Vector3>(module, "Vector3")
            .def(py::init([](const float& a, const float& b, const float& c) { return anvil::math::Vector3{a, b, c}; }))
            .def_property(
                "x", [](anvil::math::Vector3& v) { return pybind11::module_::import("numpy").attr("float32")(v.x); },
                [](anvil::math::Vector3& v, float val) { v.x = val; })
            .def_property(
                "y", [](anvil::math::Vector3& v) { return pybind11::module_::import("numpy").attr("float32")(v.y); },
                [](anvil::math::Vector3& v, float val) { v.y = val; })
            .def_property(
                "z", [](anvil::math::Vector3& v) { return pybind11::module_::import("numpy").attr("float32")(v.z); },
                [](anvil::math::Vector3& v, float val) { v.z = val; })
            .def("__add__", [](const anvil::math::Vector3& A, const anvil::math::Vector3& B) { return A + B; })
            .def("__sub__", [](const anvil::math::Vector3& A, const anvil::math::Vector3& B) { return A - B; })
            .def("__neg__", [](const anvil::math::Vector3& A) { return -A; })
            .def("__mul__",
                 [](const anvil::math::Vector3& A, const anvil::math::Vector3& B) {
                         return pybind11::module_::import("numpy").attr("float32")(A * B);
                 })
            .def("__mul__", [](const anvil::math::Vector3& A, const float SCALAR) { return A * SCALAR; })
            .def("__rmul__", [](const anvil::math::Vector3& A, const float SCALAR) { return SCALAR * A; })
            .def("__eq__", [](const anvil::math::Vector3& A, const anvil::math::Vector3& B) { return A == B; });
}
