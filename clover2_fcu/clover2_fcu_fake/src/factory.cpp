/**
 * @file factory.cpp
 * @brief Provides the fake backend factory implementation.
 */

// clover2
#include <clover2_fcu_fake/factory.hpp>

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

// third party
#include <pluginlib/class_list_macros.hpp>

// STL
#include <utility>

namespace clover2_fcu::fake {

class fake_system_state : public capability::system_state_interface {
public:
    fake_system_state(context& ctx, const YAML::Node&)
        : capability::system_state_interface(ctx) {}

    void inject(const data::system_state& state) { update_data(state); }
};

class fake_odometry : public capability::odometry_interface {
public:
    fake_odometry(context& ctx, const YAML::Node&)
        : capability::odometry_interface(ctx) {}

    void inject(const data::odometry& odom) { update_data(odom); }
};

class fake_battery : public capability::battery_interface {
public:
    fake_battery(context& ctx, const YAML::Node&)
        : capability::battery_interface(ctx) {}

    void inject(const data::battery& battery) { update_data(battery); }
};

class fake_commands : public capability::commands_interface {
public:
    explicit fake_commands(context& ctx, const YAML::Node&)
        : m_registry(ctx.get<fake_env>()->registry) {}

    result arm() override { return m_registry.log_command("arm"); }
    result disarm() override { return m_registry.log_command("disarm"); }
    result set_mode(data::flight_mode mode) override {
        return m_registry.log_command("set_mode:" +
                                      std::string(mode.to_string()));
    }
    result takeoff() override { return m_registry.log_command("takeoff"); }
    result land() override { return m_registry.log_command("land"); }
    result rtl() override { return m_registry.log_command("rtl"); }

private:
    registry& m_registry;
};

class fake_parameters : public capability::parameters_interface {
public:
    explicit fake_parameters(context& ctx, const YAML::Node&)
        : m_registry(ctx.get<fake_env>()->registry) {}

    result get(const std::string& name,
               capability::parameter_value& out) override {
        const std::scoped_lock lock(m_registry.m_mutex);
        const auto it = m_registry.parameters.find(name);
        if (it == m_registry.parameters.end()) {
            return result::value::rejected;
        }
        out = it->second;
        return result::value::success;
    }

    result set(const std::string& name,
               const capability::parameter_value& value) override {
        const std::scoped_lock lock(m_registry.m_mutex);
        m_registry.parameters[name] = value;
        return result::value::success;
    }

private:
    registry& m_registry;
};

class fake_position_setpoint : public capability::setpoint_position_interface {
public:
    fake_position_setpoint(context& ctx, const YAML::Node&)
        : capability::setpoint_position_interface(ctx),
          m_registry(ctx.get<fake_env>()->registry) {}

    void update(const Eigen::Vector3d& position,
                std::optional<double> heading = {}) override {
        const bool streaming = stream_allowed();
        if (!streaming) {
            warn_muted();
        }
        const std::scoped_lock lock(m_registry.m_mutex);
        m_registry.position_updates.push_back(
            registry::recorded_update<registry::position_payload>{
                registry::position_payload{position, heading}, streaming});
    }

private:
    registry& m_registry;
};

class fake_velocity_setpoint : public capability::setpoint_velocity_interface {
public:
    fake_velocity_setpoint(context& ctx, const YAML::Node&)
        : capability::setpoint_velocity_interface(ctx),
          m_registry(ctx.get<fake_env>()->registry) {}

    void update(const Eigen::Vector3d& velocity,
                std::optional<double> yaw_rate = {}) override {
        const bool streaming = stream_allowed();
        if (!streaming) {
            warn_muted();
        }
        const std::scoped_lock lock(m_registry.m_mutex);
        m_registry.velocity_updates.push_back(
            registry::recorded_update<registry::velocity_payload>{
                registry::velocity_payload{velocity, yaw_rate}, streaming});
    }

private:
    registry& m_registry;
};

class fake_attitude_setpoint : public capability::setpoint_attitude_interface {
public:
    fake_attitude_setpoint(context& ctx, const YAML::Node&)
        : capability::setpoint_attitude_interface(ctx),
          m_registry(ctx.get<fake_env>()->registry) {}

    void update(const Eigen::Quaterniond& attitude, double thrust) override {
        const bool streaming = stream_allowed();
        if (!streaming) {
            warn_muted();
        }
        const std::scoped_lock lock(m_registry.m_mutex);
        m_registry.attitude_updates.push_back(
            registry::recorded_update<registry::attitude_payload>{
                registry::attitude_payload{attitude, thrust}, streaming});
    }

private:
    registry& m_registry;
};

class fake_rates_setpoint : public capability::setpoint_rates_interface {
public:
    fake_rates_setpoint(context& ctx, const YAML::Node&)
        : capability::setpoint_rates_interface(ctx),
          m_registry(ctx.get<fake_env>()->registry) {}

    void update(const Eigen::Vector3d& body_rates, double thrust) override {
        const bool streaming = stream_allowed();
        if (!streaming) {
            warn_muted();
        }
        const std::scoped_lock lock(m_registry.m_mutex);
        m_registry.rates_updates.push_back(
            registry::recorded_update<registry::rates_payload>{
                registry::rates_payload{body_rates, thrust}, streaming});
    }

private:
    registry& m_registry;
};

registry& registry::instance() {
    static registry inst;
    return inst;
}

void registry::reset() {
    const std::scoped_lock lock(m_mutex);
    parameters.clear();
    position_updates.clear();
    velocity_updates.clear();
    attitude_updates.clear();
    rates_updates.clear();
    m_command_log.clear();
    m_command_result = result::value::success;
}

void registry::publish_system_state(const data::system_state& state) {
    const std::scoped_lock lock(m_mutex);
    for (const auto& weak_iface : m_system_states) {
        if (const auto iface = weak_iface.lock()) {
            iface->inject(state);
        }
    }
}

void registry::publish_odometry(const data::odometry& odom) {
    const std::scoped_lock lock(m_mutex);
    for (const auto& weak_iface : m_odometries) {
        if (const auto iface = weak_iface.lock()) {
            iface->inject(odom);
        }
    }
}

void registry::publish_battery(const data::battery& battery) {
    const std::scoped_lock lock(m_mutex);
    for (const auto& weak_iface : m_batteries) {
        if (const auto iface = weak_iface.lock()) {
            iface->inject(battery);
        }
    }
}

void registry::register_system_state(
    const std::shared_ptr<fake_system_state>& iface) {
    const std::scoped_lock lock(m_mutex);
    m_system_states.emplace_back(iface);
}

void registry::register_odometry(const std::shared_ptr<fake_odometry>& iface) {
    const std::scoped_lock lock(m_mutex);
    m_odometries.emplace_back(iface);
}

void registry::register_battery(const std::shared_ptr<fake_battery>& iface) {
    const std::scoped_lock lock(m_mutex);
    m_batteries.emplace_back(iface);
}

void registry::set_command_result(result value) {
    const std::scoped_lock lock(m_mutex);
    m_command_result = value;
}

const std::vector<std::string> registry::command_log() const {
    const std::scoped_lock lock(m_mutex);
    return m_command_log;
}

result registry::log_command(std::string name) {
    const std::scoped_lock lock(m_mutex);
    m_command_log.push_back(std::move(name));
    return m_command_result;
}

factory::factory() {
    // Telemetry providers register into the shared test registry after
    // construction: custom creators are used instead of the standard add().
    add<fake_system_state>(
        [](context& ctx, const YAML::Node& params) -> std::shared_ptr<void> {
            auto impl = std::make_shared<fake_system_state>(ctx, params);
            registry::instance().register_system_state(impl);
            return impl;
        });
    add<fake_odometry>(
        [](context& ctx, const YAML::Node& params) -> std::shared_ptr<void> {
            auto impl = std::make_shared<fake_odometry>(ctx, params);
            registry::instance().register_odometry(impl);
            return impl;
        });
    add<fake_battery>(
        [](context& ctx, const YAML::Node& params) -> std::shared_ptr<void> {
            auto impl = std::make_shared<fake_battery>(ctx, params);
            registry::instance().register_battery(impl);
            return impl;
        });

    add<fake_commands>();
    add<fake_parameters>();
    add<fake_position_setpoint>();
    add<fake_velocity_setpoint>();
    add<fake_attitude_setpoint>();
    add<fake_rates_setpoint>();
}

std::string factory::protocol() const { return "fake"; }

void factory::do_initialize(context& ctx, const YAML::Node& params) {
    ctx.provide(std::make_shared<fake_env>(ctx, params));
}

}  // namespace clover2_fcu::fake

PLUGINLIB_EXPORT_CLASS(clover2_fcu::fake::factory,
                       clover2_fcu::backend::backend_base)
