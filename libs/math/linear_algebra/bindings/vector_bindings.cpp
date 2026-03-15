#include "math/linear_algebra/vector.hpp"
#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(math_module, module) {
        module.doc() = "Linear algebra vector module";
        py::class_<anvil::math::Vector2>(module, "Vector2")
            .def(py::init([](const anvil::f32& a, const anvil::f32& b) { return anvil::math::Vector2{a, b}; }))
            .def_property(
                "x", [](anvil::math::Vector2& v) { return pybind11::module_::import("numpy").attr("float32")(v.x); },
                [](anvil::math::Vector2& v, anvil::f32 val) { v.x = val; })
            .def_property(
                "y", [](anvil::math::Vector2& v) { return pybind11::module_::import("numpy").attr("float32")(v.y); },
                [](anvil::math::Vector2& v, anvil::f32 val) { v.y = val; })
            .def("__add__", [](const anvil::math::Vector2& a, const anvil::math::Vector2& b) { return a + b; })
            .def("__sub__", [](const anvil::math::Vector2& a, const anvil::math::Vector2& b) { return a - b; })
            .def("__neg__", [](const anvil::math::Vector2& a) { return -a; })
            .def("__mul__",
                 [](const anvil::math::Vector2& a, const anvil::math::Vector2& b) {
                         return pybind11::module_::import("numpy").attr("float32")(a * b);
                 })
            .def("__mul__", [](const anvil::math::Vector2& a, const anvil::f32 scalar) { return a * scalar; })
            .def("__rmul__", [](const anvil::math::Vector2& a, const anvil::f32 scalar) { return scalar * a; })
            .def("__eq__", [](const anvil::math::Vector2& a, const anvil::math::Vector2& b) { return a == b; });

        py::class_<anvil::math::Vector3>(module, "Vector3")
            .def(py::init([](const anvil::f32& a, const anvil::f32& b, const anvil::f32& c) {
                    return anvil::math::Vector3{a, b, c};
            }))
            .def_property(
                "x", [](anvil::math::Vector3& v) { return pybind11::module_::import("numpy").attr("float32")(v.x); },
                [](anvil::math::Vector3& v, anvil::f32 val) { v.x = val; })
            .def_property(
                "y", [](anvil::math::Vector3& v) { return pybind11::module_::import("numpy").attr("float32")(v.y); },
                [](anvil::math::Vector3& v, anvil::f32 val) { v.y = val; })
            .def_property(
                "z", [](anvil::math::Vector3& v) { return pybind11::module_::import("numpy").attr("float32")(v.z); },
                [](anvil::math::Vector3& v, anvil::f32 val) { v.z = val; })
            .def("__add__", [](const anvil::math::Vector3& a, const anvil::math::Vector3& b) { return a + b; })
            .def("__sub__", [](const anvil::math::Vector3& a, const anvil::math::Vector3& b) { return a - b; })
            .def("__neg__", [](const anvil::math::Vector3& a) { return -a; })
            .def("__mul__",
                 [](const anvil::math::Vector3& a, const anvil::math::Vector3& b) {
                         return pybind11::module_::import("numpy").attr("float32")(a * b);
                 })
            .def("__mul__", [](const anvil::math::Vector3& a, const anvil::f32 scalar) { return a * scalar; })
            .def("__rmul__", [](const anvil::math::Vector3& a, const anvil::f32 scalar) { return scalar * a; })
            .def("__eq__", [](const anvil::math::Vector3& a, const anvil::math::Vector3& b) { return a == b; });

        module.def("cross_product", &anvil::math::cross_product, py::return_value_policy::copy);
}