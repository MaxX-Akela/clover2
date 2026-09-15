/**
 * @file client.hpp
 * @brief Provides the FCU interface client.
 */

#pragma once

// clover2
#include <clover2_fcu/capability/all.hpp>
#include <clover2_fcu/capability/battery_interface.hpp>
#include <clover2_fcu/capability/commands_interface.hpp>
#include <clover2_fcu/capability/odometry_interface.hpp>
#include <clover2_fcu/capability/parameters_interface.hpp>
#include <clover2_fcu/capability/setpoint_attitude_interface.hpp>
#include <clover2_fcu/capability/setpoint_base.hpp>
#include <clover2_fcu/capability/setpoint_position_interface.hpp>
#include <clover2_fcu/capability/setpoint_rates_interface.hpp>
#include <clover2_fcu/capability/setpoint_velocity_interface.hpp>
#include <clover2_fcu/capability/system_state_interface.hpp>
#include <clover2_fcu/context.hpp>
#include <clover2_fcu/backend/backend_base.hpp>
#include <clover2_fcu_msgs/msg/fcu_config.hpp>
#include <clover2_fcu_msgs/srv/get_config.hpp>

// ROS2
#include <rclcpp/rclcpp.hpp>

// third party
#include <pluginlib/class_loader.hpp>
#include <yaml-cpp/yaml.h>

// STL
#include <chrono>
#include <concepts>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace clover2_fcu {

/**
 * @brief Setpoint capability contract served by this client build.
 *
 * Requires the type to derive from @ref capability::setpoint_base and be
 * one of the supported setpoint contracts, so unsupported types are
 * rejected at the call site with a constraint violation.
 */
template <typename T>
concept setpoint_type =
    std::derived_from<T, capability::setpoint_base> &&
    (std::same_as<T, capability::setpoint_position_interface> ||
     std::same_as<T, capability::setpoint_velocity_interface> ||
     std::same_as<T, capability::setpoint_attitude_interface> ||
     std::same_as<T, capability::setpoint_rates_interface>);

/**
 * @brief Flight stack interface client.
 *
 * Embeds into any node and fetches the configuration from the @ref bridge
 * — the single configuration point (the constructor blocks until the
 * bridge answers or the wait timeout expires, @ref exception on failure).
 *
 * The client applies the capability resolution itself: an explicit
 * override per capability if the bridge configures one, otherwise the
 * default protocol. The resolved capability-to-factory map is cached.
 *
 * Capability providers are created by get(): the factory is resolved,
 * initialized once (backend environment) and asked for a new provider
 * instance on every call — callers own the returned handles:
 *
 *   auto odometry = fcu.odometry();   // shared_ptr, keep it
 *
 * Accessors throw @ref exception when the capability is not served by the
 * resolved factory; check with @ref has (may instantiate the factory,
 * creates no providers) or declare requirements via
 * client::settings::require (validated at construction).
 */
class client : public context {
public:
    /**
     * @brief Client construction settings (fluent).
     */
    class settings {
    public:
        settings() {}

        /// @brief Bridge node name (absolute), default "/fcu_bridge".
        settings& bridge_node(std::string node_name) {
            m_bridge_node = std::move(node_name);
            return *this;
        }

        /// @brief Overall wait timeout for the bridge connection.
        template <typename rep_t, typename period_t>
        settings& wait_timeout(std::chrono::duration<rep_t, period_t> timeout) {
            m_wait_timeout =
                std::chrono::duration_cast<std::chrono::nanoseconds>(timeout);
            return *this;
        }

        /// @brief Require a capability: construction fails when its
        ///        resolved factory does not serve it.
        template <capability::contract T>
        settings& require() {
            m_required.push_back(T::name);
            return *this;
        }

        const std::string& bridge_node() const { return m_bridge_node; }
        std::chrono::nanoseconds wait_timeout() const { return m_wait_timeout; }
        const std::vector<std::string>& required() const { return m_required; }

    private:
        std::string m_bridge_node{"/fcu_bridge"};
        std::vector<std::string> m_required{};
        std::chrono::nanoseconds m_wait_timeout{std::chrono::seconds(10)};
    };

    /**
     * @brief Construct the client and connect it to the bridge.
     * @tparam NodeT Node type (clover2_common::node, lifecycle node, ...).
     * @param node the hosting node (must be spun while this runs).
     * @param s client settings.
     * @throws clover2_fcu::exception on connection or requirement failures.
     */
    template <typename NodeT>
    explicit client(NodeT& node, settings s = {})
        : context(node)
        , m_settings(std::move(s)) {
        m_callback_group = node.create_callback_group(
            rclcpp::CallbackGroupType::MutuallyExclusive);

        m_config_client =
            node.template create_client<clover2_fcu_msgs::srv::GetConfig>(
                m_settings.bridge_node() + "/get_config", rclcpp::ServicesQoS(),
                m_callback_group);

        connect();
    }

    client(const client&) = delete;
    client& operator=(const client&) = delete;
    ~client() = default;

    template <capability::contract CapabilityT>
    CapabilityT::shared_ptr get() {
        return std::static_pointer_cast<CapabilityT>(get(CapabilityT::name));
    }

    std::shared_ptr<void> get(const std::string& capability);

    capability::system_state_interface::shared_ptr state() {
        return get<capability::system_state_interface>();
    }

    capability::odometry_interface::shared_ptr odometry() {
        return get<capability::odometry_interface>();
    }

    capability::battery_interface::shared_ptr battery() {
        return get<capability::battery_interface>();
    }

    capability::commands_interface::shared_ptr commands() {
        return get<capability::commands_interface>();
    }

    capability::parameters_interface::shared_ptr parameters() {
        return get<capability::parameters_interface>();
    }

    /**
     * @brief Setpoint capability accessor.
     * @tparam T supported setpoint contract
     *         (capability::setpoint_position_interface, ...).
     * @return a new provider handle; callers own it.
     */
    template <setpoint_type T>
    T::shared_ptr setpoint() {
        if constexpr (std::same_as<T,
                                   capability::setpoint_position_interface>) {
            return get<capability::setpoint_position_interface>();
        } else if constexpr (std::same_as<
                                 T, capability::setpoint_velocity_interface>) {
            return get<capability::setpoint_velocity_interface>();
        } else if constexpr (std::same_as<
                                 T, capability::setpoint_attitude_interface>) {
            return get<capability::setpoint_attitude_interface>();
        } else if constexpr (std::same_as<
                                 T, capability::setpoint_rates_interface>) {
            return get<capability::setpoint_rates_interface>();
        }
    }

    /// @brief Whether the capability is served by its resolved factory.
    ///
    /// Resolves the factory (may instantiate it; creates no providers).
    template <capability::contract T>
    bool has() {
        return has(T::name);
    }

    /// @brief Whether the capability is served by its resolved factory.
    bool has(const std::string& capability);

    /// @brief The client callback group (used by mode_base for its own
    ///        clients/subscriptions/timers).
    rclcpp::CallbackGroup::SharedPtr callback_group() const {
        return m_callback_group;
    }

private:
    /// @brief Blocking bridge connection.
    /// @throws clover2_fcu::exception on failure.
    void connect();

    /// @brief Cache the configuration and validate requirements.
    void apply_config(const clover2_fcu_msgs::msg::FcuConfig& config);

    /// @brief Fail when a required capability is not served.
    void check_requirements();

    /// @brief The configured backend, initialized once.
    /// @throws clover2_fcu::exception on failure.
    backend::backend_base::shared_ptr backend();

    /// @brief Load (and cache) a backend by plugin name.
    /// @throws clover2_fcu::exception on failure.
    backend::backend_base::shared_ptr ensure_backend(
        const std::string& plugin_name);

    settings m_settings;

    rclcpp::CallbackGroup::SharedPtr m_callback_group;
    rclcpp::Client<clover2_fcu_msgs::srv::GetConfig>::SharedPtr m_config_client;

    YAML::Node m_params;
    clover2_fcu_msgs::msg::FcuConfig m_config;

    pluginlib::ClassLoader<backend::backend_base> m_loader{
        "clover2_fcu", "clover2_fcu::backend::backend_base"};
    std::unordered_map<std::string, backend::backend_base::shared_ptr>
        m_backends;
};

}  // namespace clover2_fcu
