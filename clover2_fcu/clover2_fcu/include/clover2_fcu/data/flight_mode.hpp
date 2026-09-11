#pragma once

// STL
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace clover2_fcu::data {

class flight_mode {
public:
    enum class value : uint8_t {
        manual = 0,
        acrobatic,
        stabilize,
        altitude_hold,
        position_hold,
        loiter,
        offboard,
        auto_mission,
        auto_takeoff,
        auto_land,
        rtl,
        unknown,
    };

    flight_mode() noexcept = default;
    flight_mode(value v) noexcept
        : m_value(v) {}

    operator value() const noexcept { return m_value; }
    operator std::string_view() const noexcept { return to_string(); }

    std::string_view to_string() const noexcept;

    static std::optional<flight_mode> from_string(const std::string& name);

private:
    value m_value{value::unknown};
};

}  // namespace clover2_fcu::data
