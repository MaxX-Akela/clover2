/**
 * @file context.hpp
 * @brief Provides the FCU interface layer context.
 */

#pragma once

// clover2
#include <clover2_common/node_context.hpp>
#include <clover2_fcu/common/exception.hpp>

// ROS2
#include <rclcpp/rclcpp.hpp>

// STL
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace clover2_fcu {

/**
 * @brief Client runtime info published into the context by the client;
 *        capability providers use it to reach the same bridge (e.g. the
 *        setpoint gate subscribes to the bridge mode_state).
 */
struct client_runtime {
    std::string bridge_node;
};

/**
 * @brief Context passed to capability providers.
 *
 * Extends the project node context with type-indexed backend extensions:
 * the client publishes its runtime (client_runtime), a backend factory
 * publishes its shared environment from do_initialize() with provide<T>(),
 * providers reach it with get<T>(). mode_base publishes a mode_token that
 * binds setpoint providers to the mode.
 *
 * Single-threaded executor is assumed.
 */
class context : public clover2_common::node_context {
public:
    /**
     * @brief Construct the context from a node.
     * @tparam node_t Node type (clover2_common::node, lifecycle node, ...).
     * @param node Node instance.
     */
    template <typename node_t>
    explicit context(node_t& node)
        : clover2_common::node_context(node) {}

    /** @brief Get the node clock. */
    rclcpp::Clock::SharedPtr clock() {
        return get_node_clock_interface()->get_clock();
    }

    /** @brief Publish an extension (client runtime, backend environment,
     *        mode token). */
    template <typename T>
    void provide(std::shared_ptr<T> extension) {
        m_extensions[std::type_index(typeid(T))] = std::move(extension);
    }

    /** @brief Get an extension published with provide<T>().
     * @throws clover2_fcu::exception when the extension was not provided. */
    template <typename T>
    std::shared_ptr<T> get() const {
        const auto it = m_extensions.find(std::type_index(typeid(T)));
        if (it == m_extensions.end()) {
            throw exception("extension not provided: {}", typeid(T).name());
        }

        return std::static_pointer_cast<T>(it->second);
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_extensions;
};

}  // namespace clover2_fcu
