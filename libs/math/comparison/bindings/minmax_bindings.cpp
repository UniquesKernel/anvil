#include "anvil/types.hpp"
#include "comparison.hpp"
#include "math/comparison/comparison.hpp"
#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_minmax(pybind11::module_& module) {
        auto m  = module.def_submodule("math_comparison");

        m.doc() = "";

        m.def(
            "min_int",
            [](const anvil::u64 a, const anvil::u64 b) { return anvil::math::comparison::min(a, b); },
            py::arg("a"), py::arg("b"));

        m.def(
            "max_int",
            [](const anvil::u64 a, const anvil::u64 b) { return anvil::math::comparison::max(a, b); },
            py::arg("a"), py::arg("b"));
}
