#pragma once

// clover2
#include <clover2_fcu/data/flight_mode.hpp>

namespace clover2_fcu::data {

struct system_state {
    bool connected{false};
    bool armed{false};
    bool in_air{false};
    bool failsafe{false};
    flight_mode mode{flight_mode::value::unknown};
};

}  // namespace clover2_fcu::data
