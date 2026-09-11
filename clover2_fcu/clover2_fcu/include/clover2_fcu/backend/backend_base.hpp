/**
 * @file backend_base.hpp
 * @brief Provides the backend base class (pluginlib base).
 */

#pragma once

// clover2
#include <clover2_fcu/capability/all.hpp>
#include <clover2_fcu/common/exception.hpp>
#include <clover2_fcu/context.hpp>

// third party
#include <yaml-cpp/yaml.h>

// STL
#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace clover2_fcu::backend {

/**
 * @brief Capability provider: derives from the contract recovered from the
 *        inherited shared_ptr alias (so the registration key from the
 *        inherited name always matches the real contract), is not the
 *        contract itself, and is constructible from (context&, params).
 */
template <typename T>
concept is_capability =  //
    capability::contract<T> &&
    std::derived_from<T, typename T::shared_ptr::element_type> &&
    !std::same_as<T, typename T::shared_ptr::element_type> &&
    std::constructible_from<T, context&, const YAML::Node&>;

/**
 * @brief Base class of a backend (pluginlib base).
 *
 * One pluginlib class per backend package; the lookup name must equal
 * protocol() (validated by the bridge). Providers are registered as
 * name-keyed creators via add() in the derived constructor, following the
 * project-wide factory pattern (see clover2_led::animation::factory).
 *
 * Lifecycle: the client calls initialize() once before the first create();
 * backends publish their shared environment into the context from
 * do_initialize() (see context::provide/get). The bridge reads only
 * protocol()/capabilities() and never initializes the backend.
 *
 * Single-threaded executor is assumed.
 */
class backend_base {
public:
    using shared_ptr = std::shared_ptr<backend_base>;
    using unique_ptr = std::unique_ptr<backend_base>;

    /// @brief Creates one capability provider.
    using builder_type = std::function<std::shared_ptr<void>(
        context& ctx, const YAML::Node& params)>;

    virtual ~backend_base() = default;

    /// @brief Backend (protocol) name used as the default plugin.
    virtual std::string protocol() const = 0;

    /// @brief Mode registration hook, called by the bridge from its
    ///        register_mode service.
    ///
    /// Backends whose FCU supports native external-mode registration
    /// duplicate it here (e.g. px4 RegisterExtComponent); the default
    /// does nothing — the bridge lease is the only arbiter.
    /// @return backend-native mode id, 0 when the backend has none.
    virtual uint32_t register_mode(const std::string& name) {
        (void)name;
        return 0;
    }

    /// @brief Mode unregistration hook, called by the bridge when a mode is
    ///        removed (bond broken / cleanup). Default does nothing.
    virtual void unregister_mode(const std::string& name) { (void)name; }

    /// @brief Names of the capabilities registered by this backend.
    std::vector<std::string> capabilities() const;

    void initialize(context& ctx, const YAML::Node& params);

    /**
     * @brief Create the provider for the capability name.
     * @throws clover2_fcu::exception when the factory is not loaded or
     *         the capability is not registered.
     */
    std::shared_ptr<void> create(const std::string& capability, context& ctx,
                                 const YAML::Node& params) const;

    /**
     * @brief Create the provider for the capability contract.
     * @tparam CapabilityT capability contract
     *         (capability::odometry_interface, ...).
     * @throws clover2_fcu::exception when the factory is not loaded or
     *         the capability is not registered.
     */
    template <capability::contract CapabilityT>
    std::shared_ptr<CapabilityT> create(context& ctx,
                                        const YAML::Node& params) const {
        return std::static_pointer_cast<CapabilityT>(
            create(CapabilityT::name, ctx, params));
    }

protected:
    /**
     * @brief Register a provider for the capability it derives from.
     *
     * The contract type is recovered from the inherited T::shared_ptr
     * alias; the registration key is the inherited T::name.
     *
     * @tparam T provider, constructible from (context&, const YAML::Node&).
     */
    template <is_capability T>
    void add() {
        add<T>([](context& ctx, const YAML::Node& params) {
            return std::make_shared<T>(ctx, params);
        });
    }

    template <is_capability T>
    void add(builder_type&& builder) {
        m_builders.emplace(T::name, std::move(builder));
    }

    virtual void do_initialize(context& ctx, const YAML::Node& params) = 0;

private:
    std::unordered_map<std::string, builder_type> m_builders;
    bool m_loaded{false};
};

}  // namespace clover2_fcu::backend
