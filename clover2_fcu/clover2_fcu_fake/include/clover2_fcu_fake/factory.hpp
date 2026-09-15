/**
 * @file factory.hpp
 * @brief Provides the fake backend factory and its test-facing registry.
 */

#pragma once

// clover2
#include <clover2_fcu/capability/parameters_interface.hpp>
#include <clover2_fcu/context.hpp>
#include <clover2_fcu/data/battery.hpp>
#include <clover2_fcu/data/odometry.hpp>
#include <clover2_fcu/data/system_state.hpp>
#include <clover2_fcu/backend/backend_base.hpp>

// third party
#include <Eigen/Geometry>

// STL
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace clover2_fcu::fake {

class fake_system_state;
class fake_odometry;
class fake_battery;

/**
 * @brief Test-facing registry of live fake providers.
 *
 * The fake backend keeps no transport: tests inject telemetry snapshots
 * and observe commands/setpoints through this singleton. Live telemetry
 * providers are tracked with weak pointers; entries expire together with
 * the owning client. It is shared by all fake providers across all
 * clients in the process.
 */
class registry {
public:
    /// @brief A recorded setpoint update with the gate state at the call.
    template <typename payload_t>
    struct recorded_update {
        payload_t payload;
        bool was_streaming{false};
    };

    struct position_payload {
        Eigen::Vector3d position{Eigen::Vector3d::Zero()};
        std::optional<double> heading{};
    };

    struct velocity_payload {
        Eigen::Vector3d velocity{Eigen::Vector3d::Zero()};
        std::optional<double> yaw_rate{};
    };

    struct attitude_payload {
        Eigen::Quaterniond attitude{Eigen::Quaterniond::Identity()};
        double thrust{0.0};
    };

    struct rates_payload {
        Eigen::Vector3d body_rates{Eigen::Vector3d::Zero()};
        double thrust{0.0};
    };

    static registry& instance();

    /// @brief Reset all logs, results and parameters between tests.
    void reset();

    // Telemetry injection (reaches every live fake telemetry provider).

    void publish_system_state(const data::system_state& state);
    void publish_odometry(const data::odometry& odom);
    void publish_battery(const data::battery& battery);

    // Commands behavior and observation.

    /// @brief Sticky result returned by subsequent fake command calls.
    void set_command_result(result value);

    /// @brief Command names in call order ("arm", "land", ...).
    const std::vector<std::string> command_log() const;

    // Parameters storage (shared by the fake parameters provider).

    std::map<std::string, capability::parameter_value> parameters;

    // Setpoint observation.

    std::vector<recorded_update<position_payload>> position_updates;
    std::vector<recorded_update<velocity_payload>> velocity_updates;
    std::vector<recorded_update<attitude_payload>> attitude_updates;
    std::vector<recorded_update<rates_payload>> rates_updates;

private:
    friend class factory;
    friend class fake_commands;
    friend class fake_parameters;
    friend class fake_position_setpoint;
    friend class fake_velocity_setpoint;
    friend class fake_attitude_setpoint;
    friend class fake_rates_setpoint;

    registry() = default;

    void register_system_state(const std::shared_ptr<fake_system_state>& iface);
    void register_odometry(const std::shared_ptr<fake_odometry>& iface);
    void register_battery(const std::shared_ptr<fake_battery>& iface);

    result log_command(std::string name);

    mutable std::mutex m_mutex;
    std::vector<std::weak_ptr<fake_system_state>> m_system_states;
    std::vector<std::weak_ptr<fake_odometry>> m_odometries;
    std::vector<std::weak_ptr<fake_battery>> m_batteries;
    std::vector<std::string> m_command_log;
    result m_command_result{result::value::success};
};

/**
 * @brief Fake backend environment, published as a context extension.
 *
 * Providers reach the shared test registry through it:
 * `ctx.get<fake_env>().registry`.
 */
struct fake_env {
    class registry& registry;

    explicit fake_env(context&, const YAML::Node&)
        : registry(clover2_fcu::fake::registry::instance()) {}
};

/**
 * @brief In-memory backend factory ("fake" protocol).
 *
 * Serves every MVP capability. Useful for tests and for developing
 * against a simulated flight stack without any transport.
 */
class factory : public clover2_fcu::backend::backend_base {
public:
    factory();

    std::string protocol() const override;

    void do_initialize(context& ctx, const YAML::Node& params) override;
};

}  // namespace clover2_fcu::fake
