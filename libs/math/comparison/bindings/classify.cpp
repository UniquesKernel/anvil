#include "anvil/types.hpp"
#include "comparison.hpp"
#include "anvil/math/comparison/comparison.hpp"
#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_classify(pybind11::module_& module) {
        auto m = module.def_submodule("math_classify");

        py::enum_<anvil::FloatType>(m, "FloatType")
            .value("POS_FINITE", anvil::POS_FINITE)
            .value("POS_INF", anvil::POS_INF)
            .value("POS_NAN", anvil::POS_NAN)
            .value("NEG_FINITE", anvil::NEG_FINITE)
            .value("NEG_INF", anvil::NEG_INF)
            .value("NEG_NAN", anvil::NEG_NAN);

        m.def("classify", &anvil::math::comparison::classify, py::arg("num"));
}
