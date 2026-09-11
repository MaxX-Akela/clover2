// clover2
#include <clover2_fcu/data/flight_mode.hpp>

// STL
#include <string>
#include <string_view>
#include <unordered_map>

namespace clover2_fcu::data {

std::string_view flight_mode::to_string() const noexcept {
    static const std::unordered_map<value, const char*> k_names = {
        {value::manual, "manual"},
        {value::acrobatic, "acrobatic"},
        {value::stabilize, "stabilize"},
        {value::altitude_hold, "altitude_hold"},
        {value::position_hold, "position_hold"},
        {value::loiter, "loiter"},
        {value::offboard, "offboard"},
        {value::auto_mission, "auto_mission"},
        {value::auto_takeoff, "auto_takeoff"},
        {value::auto_land, "auto_land"},
        {value::rtl, "rtl"},
        {value::unknown, "unknown"},
    };

    const auto it = k_names.find(m_value);
    return it != k_names.end() ? it->second : "unknown";
}

std::optional<flight_mode> flight_mode::from_string(const std::string& name) {
    static const std::unordered_map<std::string, value> k_values = {
        {"manual", value::manual},
        {"acrobatic", value::acrobatic},
        {"stabilize", value::stabilize},
        {"altitude_hold", value::altitude_hold},
        {"position_hold", value::position_hold},
        {"loiter", value::loiter},
        {"offboard", value::offboard},
        {"auto_mission", value::auto_mission},
        {"auto_takeoff", value::auto_takeoff},
        {"auto_land", value::auto_land},
        {"rtl", value::rtl},
        {"unknown", value::unknown},
    };

    const auto it = k_values.find(name);
    if (it == k_values.end()) {
        return std::nullopt;
    }
    return flight_mode{it->second};
}

}  // namespace clover2_fcu::data
