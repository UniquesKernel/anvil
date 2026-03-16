#include "anvil/types.hpp"
#include "comparison.hpp"
#include "math/comparison/comparison.hpp"
#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_comparison(pybind11::module_& module) {
        auto m  = module.def_submodule("math_comparison");

        m.doc() = "";

        m.def(
            "min_int",
            [](const anvil::u64 left, const anvil::u64 right) { return anvil::math::comparison::min(left, right); },
            py::arg("left"), py::arg("right"));

        m.def(
            "max_int",
            [](const anvil::u64 left, const anvil::u64 right) { return anvil::math::comparison::max(left, right); },
            py::arg("left"), py::arg("right"));
}
