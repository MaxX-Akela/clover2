/**
 * @file setpoint_base.hpp
 * @brief Provides the setpoint capability base and the mode gate.
 */

#pragma once

// clover2
#include <clover2_fcu/common/exception.hpp>
#include <clover2_fcu/context.hpp>
#include <clover2_fcu_msgs/msg/mode_state.hpp>

// ROS2
#include <rclcpp/rclcpp.hpp>

// STL
#include <memory>

namespace clover2_fcu {

/**
 * @brief Mode token: published into the context by mode_base at
 *        construction; a setpoint provider created while a token is
 *        present is bound to that mode.
 */
struct mode_token {
    uint32_t mode_id{0};
};

namespace capability {

/**
 * @brief Base class for setpoint capability contracts and providers.
 *
 * Creating a setpoint provider is always allowed (px4 parity: px4 lets you
 * construct setpoint types anywhere too), publishing is not:
 *
 * - a provider created inside a mode (mode_base publishes a @ref mode_token
 *   into the context) streams only while that mode holds the bridge lease;
 * - a provider created outside any mode is permanently muted.
 *
 * Backend providers must check stream_allowed() before every publish and
 * silently skip the publication when it is false (mute, not an exception) —
 * this is the counterpart of px4's "the FMU consumes only the active
 * mode's stream" arbitration.
 */
class setpoint_base {
public:
    explicit setpoint_base(context& ctx);
    setpoint_base(const setpoint_base&) = delete;
    setpoint_base& operator=(const setpoint_base&) = delete;
    virtual ~setpoint_base() = default;

    /// @brief Whether publishing is currently allowed for this provider.
    bool stream_allowed() const noexcept {
        return m_mode_id != 0 && m_active_mode_id == m_mode_id;
    }

    /// @brief Log a throttled warning when publishing is attempted while
    /// muted. Call when stream_allowed() returned false.
    void warn_muted() const {
        RCLCPP_WARN_THROTTLE(
            m_logger, *m_clock, 5000,
            "setpoint provider is muted (%s); stream setpoints via "
            "clover2_fcu::mode_base",
            m_mode_id == 0 ? "created outside a mode" : "mode not active");
    }

protected:
    context& m_ctx;

private:
    rclcpp::Logger m_logger{rclcpp::get_logger("clover2_fcu_setpoint")};
    rclcpp::Clock::SharedPtr m_clock;
    uint32_t m_mode_id{0};
    uint32_t m_active_mode_id{0};
    rclcpp::Subscription<clover2_fcu_msgs::msg::ModeState>::SharedPtr
        m_mode_state_sub;
};

}  // namespace capability

}  // namespace clover2_fcu
