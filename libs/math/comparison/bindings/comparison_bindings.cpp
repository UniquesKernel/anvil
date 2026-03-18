#include "comparison.hpp"
#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(anvil_math, module) {
        module.doc() = "Anvil Mathematics Module";

        bind_minmax(module);
        bind_classify(module);
}
