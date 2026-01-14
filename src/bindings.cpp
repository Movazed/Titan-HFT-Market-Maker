#include <pybind11/pybind11.h>
#include <pybind11/stl.h> //using this for converion of stl vectors into python lists...
#include "strategies/AvellanedaStoikov.hpp"

namespace py = pybind11;

// we create a class for making the python accessible for this code....

class StrategyWrapper{
public:
    strategy::AvellanedaStoikov engine;

    // making a helper function so that it returns into python dictionary list instead of C++ struct this makes it easy to read in python

    py::dict get_quote(double mid_price, int inventory, double volatality){
        auto q = engine.calculate_quote(mid_price, inventory, volatality);

        py::dict result;
        result["bid"] = q.bid_price;
        result["ask"] = q.ask_price;
        result["is_panic"] = q.panic_mode;
        return result;
    }
};

//now this will define the python module core or hft core

PYBIND11_MODULE(hft_core, m) {
    m.doc() = "C++ engine wrapped for python backtesting..";

    py::class_<StrategyWrapper>(m, "Strategy")
        .def(py::init<>()) // default constructor
        .def("get_quote", &StrategyWrapper::get_quote, "calculate bid/ask quotes based on market data");
}

// now we gonna run the binder 
