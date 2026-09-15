#pragma once

#include <rclcpp/subscription.hpp>

namespace clover2_common::util {

template <typename MessageT, typename AllocatorT = std::allocator<void>>
class shared_subscription : public std::enable_shared_from_this<
                                shared_subscription<MessageT, AllocatorT>> {
public:
    using SubscriptionT = rclcpp::Subscription<MessageT, AllocatorT>;

    using callback_token = size_t;
    using callback_instance =
        std::unique_ptr<const unsigned, std::function<void(const unsigned*)>>;

    explicit shared_subscription(
        rclcpp::AnySubscriptionCallback<MessageT, AllocatorT> cb) {
        // rclcpp::QoS(1)
    }

    const typename SubscriptionT::SharedPtr& get_subscription() const {
        return m_sub;
    }

private:
    typename SubscriptionT::SharedPtr m_sub;
    
};

}  // namespace clover2_common::util
