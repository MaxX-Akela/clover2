/**
 * @file commands_interface.hpp
 * @brief Provides the vehicle commands capability contract.
 */

#pragma once

// clover2
#include <clover2_fcu/common/result.hpp>
#include <clover2_fcu/data/flight_mode.hpp>

// STL
#include <memory>

namespace clover2_fcu::capability {

/**
 * @brief Vehicle commands capability contract
 *        (arming, modes, navigation actions).
 *
 * All methods are synchronous request/acknowledge calls; outcomes are
 * reported via @ref clover2_fcu::result, not exceptions.
 */
class commands_interface {
public:
    using shared_ptr = std::shared_ptr<commands_interface>;

    static constexpr const char* name = "commands";

    virtual ~commands_interface() = default;

    /// @brief Arm the vehicle.
    virtual result arm() = 0;

    /// @brief Disarm the vehicle.
    virtual result disarm() = 0;

    /// @brief Switch the flight mode.
    virtual result set_mode(data::flight_mode mode) = 0;

    /// @brief Trigger an automatic takeoff.
    virtual result takeoff() = 0;

    /// @brief Trigger an automatic landing.
    virtual result land() = 0;

    /// @brief Trigger a return-to-launch.
    virtual result rtl() = 0;
};

}  // namespace clover2_fcu::capability
