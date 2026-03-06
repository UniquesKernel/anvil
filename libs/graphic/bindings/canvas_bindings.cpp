
#include "graphic/canvas.hpp"
#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(graphic_module, module) {
        module.doc();
        module.attr("MAX_HEIGHT") = py::size_t(anvil::graphic::MAX_HEIGHT);
        module.attr("MAX_WIDTH")  = py::size_t(anvil::graphic::MAX_WIDTH);

        py::class_<anvil::graphic::Canvas>(module, "Canvas")
            .def(py::init([](const unsigned long width, const unsigned long height, const char fill_char) {
                    anvil::graphic::Canvas canvas;
                    (void)anvil::graphic::create(&canvas, width, height, fill_char);
                    return canvas;
            }))
            .def_property(
                "height", [](const anvil::graphic::Canvas& canvas) { return canvas.height; },
                [](anvil::graphic::Canvas& canvas, unsigned long val) { canvas.height = val; })
            .def_property(
                "width", [](const anvil::graphic::Canvas& canvas) { return canvas.width; },
                [](anvil::graphic::Canvas& canvas, unsigned long val) { canvas.width = val; })
            .def_property(
                "buffer", [](const anvil::graphic::Canvas& canvas) { return canvas.buffer; }, []() {})
            .def_property("allocator", [](const anvil::graphic::Canvas& canvas) { return canvas.allocator; }, []() {});
}
