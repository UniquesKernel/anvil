#include <pybind11/pybind11.h>
#include "sample/example.hpp"

namespace py = pybind11;

PYBIND11_MODULE(example_module, module) {
  module.doc() = "Example module bindings";
  module.def("example", &anvil::sample::example, "Return a minimal example value");
}
