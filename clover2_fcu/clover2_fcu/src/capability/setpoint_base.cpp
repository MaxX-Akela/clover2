// clover2
#include <clover2_fcu/capability/setpoint_base.hpp>

namespace clover2_fcu::capability {

setpoint_base::setpoint_base(context& ctx)
    : m_ctx(ctx), m_logger(ctx.get_logger()), m_clock(ctx.clock()) {
    // Bridge node name follows the client that owns this context.
    std::string bridge_node = "/fcu_bridge";
    try {
        bridge_node = ctx.get<client_runtime>()->bridge_node;
    } catch (const exception&) {
        // No client_runtime published: the default bridge node name.
    }

    // Providers created inside a mode are bound to it (mode_base publishes
    // the token before creating providers). Outside a mode: permanently
    // muted (px4 parity — creation is free, streaming is not).
    try {
        m_mode_id = ctx.get<mode_token>()->mode_id;
    } catch (const exception&) {
        RCLCPP_WARN_ONCE(
            m_logger,
            "setpoint provider created outside mode_base: publishing is "
            "disabled");
    }

    m_mode_state_sub = rclcpp::create_subscription<
        clover2_fcu_msgs::msg::ModeState>(
        ctx, bridge_node + "/mode_state", rclcpp::QoS(1).transient_local(),
        [this](clover2_fcu_msgs::msg::ModeState::UniquePtr msg) {
            m_active_mode_id = msg->mode_id;
        });
}

}  // namespace clover2_fcu::capability
