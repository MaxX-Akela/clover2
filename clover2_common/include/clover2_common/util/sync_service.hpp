/**
 * @file sync_service.hpp
 * @brief Provides a blocking service call helper.
 */

#pragma once

// ROS2
#include <rclcpp/rclcpp.hpp>

// STL
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

namespace clover2_common::util {

/**
 * @brief Call a service and block until the response arrives.
 *
 * The response is delivered through the client's callback group, so the
 * hosting node must be spun while this runs. Do not call from inside a
 * callback of the executor that spins the node (would deadlock).
 *
 * @throws std::runtime_error on timeout.
 */
template <typename service_t>
std::shared_ptr<typename service_t::Response> call_sync(
    const std::shared_ptr<rclcpp::Client<service_t>>& client,
    const std::string& service_name,
    const std::shared_ptr<typename service_t::Request>& request,
    std::chrono::nanoseconds timeout) {
    auto future = client->async_send_request(request);

    if (future.wait_for(timeout) != std::future_status::ready) {
        client->remove_pending_request(future);
        throw std::runtime_error("service '" + service_name +
                                 "' did not answer in time");
    }

    auto response = future.get();
    client->remove_pending_request(future);
    return response;
}

}  // namespace clover2_common::util
