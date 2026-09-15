/**
 * @file all.hpp
 * @brief Provides the single list of all capability contracts.
 */

#pragma once

// clover2
#include <clover2_fcu/capability/battery_interface.hpp>
#include <clover2_fcu/capability/commands_interface.hpp>
#include <clover2_fcu/capability/odometry_interface.hpp>
#include <clover2_fcu/capability/parameters_interface.hpp>
#include <clover2_fcu/capability/setpoint_attitude_interface.hpp>
#include <clover2_fcu/capability/setpoint_position_interface.hpp>
#include <clover2_fcu/capability/setpoint_rates_interface.hpp>
#include <clover2_fcu/capability/setpoint_velocity_interface.hpp>
#include <clover2_fcu/capability/system_state_interface.hpp>

// STL
#include <concepts>
#include <string>
#include <tuple>
#include <vector>

namespace clover2_fcu::capability {

/**
 * @brief Capability contract: carries the registration name and the
 *        shared_ptr alias (both inherited by providers).
 */
template <typename T>
concept contract = requires {
    typename T::shared_ptr;
    { T::name } -> std::convertible_to<const char*>;
};

/**
 * @brief The single list of all capability contracts.
 *
 * Names are owned by the contracts (T::name); this tuple is the only
 * place enumerating them. Adding a capability = new contract header plus
 * one line here; the compile-time uniqueness check below then guards the
 * whole list.
 */
using all = std::tuple<system_state_interface,       //
                       odometry_interface,           //
                       battery_interface,            //
                       commands_interface,           //
                       parameters_interface,         //
                       setpoint_position_interface,  //
                       setpoint_velocity_interface,  //
                       setpoint_attitude_interface,  //
                       setpoint_rates_interface>;

namespace detail {

constexpr bool names_equal(const char* a, const char* b);

template <typename... Ts>
consteval bool names_unique() {
    const char* names[] = {Ts::name...};

    for (std::size_t i = 0; i < sizeof...(Ts); ++i) {
        for (std::size_t j = i + 1; j < sizeof...(Ts); ++j) {
            if (names_equal(names[i], names[j])) {
                return false;
            }
        }
    }

    return true;
}

}  // namespace detail

/**
 * @brief Names of all capability contracts (from @ref all).
 */
const std::vector<std::string>& all_names();

}  // namespace clover2_fcu::capability
