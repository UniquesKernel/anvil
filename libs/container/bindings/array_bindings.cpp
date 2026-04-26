#include "anvil/container/array.hpp"
#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(anvil_container, m) {
        m.doc() = "Anvil Container Library";
}
