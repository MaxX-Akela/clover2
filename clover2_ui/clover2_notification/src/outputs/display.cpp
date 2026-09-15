#include <arpa/inet.h>
#include <clover2_common/node_context.hpp>
#include <clover2_common/util/timer.hpp>
#include <clover2_display/client.hpp>
#include <clover2_notification/data/priority.hpp>
#include <clover2_notification/display_screen.hpp>
#include <clover2_notification/output.hpp>
#include <netinet/in.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <pluginlib/class_list_macros.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/msg/image.hpp>

#include <chrono>
#include <cstdint>
#include <ifaddrs.h>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace clover2_notification::outputs {

namespace {

constexpr int k_default_width = 128;
constexpr int k_default_height = 64;

std::string get_hostname() {
    char hostname[256]{};
    if (::gethostname(hostname, sizeof(hostname) - 1) != 0) {
        return "unknown";
    }
    hostname[sizeof(hostname) - 1] = '\0';
    return hostname;
}

std::string get_network_address(const std::vector<std::string>& interfaces) {
    ifaddrs* addresses{};
    if (::getifaddrs(&addresses) != 0) {
        return "n/a";
    }

    for (const auto& interface : interfaces) {
        for (auto* address = addresses; address != nullptr;
             address = address->ifa_next) {
            if (address->ifa_name == nullptr || address->ifa_addr == nullptr ||
                interface != address->ifa_name ||
                address->ifa_addr->sa_family != AF_INET) {
                continue;
            }

            char buffer[INET_ADDRSTRLEN]{};
            const auto* ipv4 =
                reinterpret_cast<const sockaddr_in*>(address->ifa_addr);
            if (::inet_ntop(AF_INET, &ipv4->sin_addr, buffer, sizeof(buffer))) {
                ::freeifaddrs(addresses);
                return buffer;
            }
        }
    }

    ::freeifaddrs(addresses);
    return "n/a";
}

}  // namespace

/** @brief Notification output that renders a status screen to a display. */
class display final : public clover2_notification::output {
public:
    display() = default;
    ~display() override = default;

    void clear() override {
        if (m_refresh_timer) {
            m_refresh_timer->cancel();
            m_refresh_timer.reset();
        }
        if (m_screen) {
            m_screen->clear();
        }
        output::clear();
        render_and_send();
    }

private:
    void on_initialize() override {
        m_logger = node_context()
                       ->get_logger()
                       .get_child("display_output")
                       .get_child(id());
        m_base_path =
            declare_output_parameter<std::string>("base_path", m_base_path);
        m_refresh_period = declare_output_parameter<double>("refresh_period",
                                                            m_refresh_period);
        m_parameters = declare_output_parameter<std::vector<std::string>>(
            "parameters", m_parameters);
        m_network_interfaces =
            declare_output_parameter<std::vector<std::string>>(
                "network.interfaces", m_network_interfaces);
        m_font.scale =
            declare_output_parameter<double>("font.scale", m_font.scale);
        m_font.thickness =
            declare_output_parameter<int>("font.thickness", m_font.thickness);
        m_alert_enabled =
            declare_output_parameter<bool>("alert.enabled", m_alert_enabled);

        if (m_refresh_period <= 0.0 || m_font.scale <= 0.0 ||
            m_font.thickness <= 0) {
            throw std::invalid_argument("Invalid display configuration");
        }

        m_screen = std::make_unique<display_screen>(m_font, m_parameters);
        m_client = std::make_shared<clover2_display::client>(node_context(),
                                                             m_base_path);

        const auto refresh_period =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::duration<double>(m_refresh_period));
        m_refresh_timer = clover2_common::util::create_timer(
            node_context(), refresh_period, [this]() { render_and_send(); });

        RCLCPP_INFO(m_logger,
                    "Display output initialized: base_path='%s' "
                    "refresh_period=%.2fs parameters=%zu alert=%s",
                    m_base_path.c_str(), m_refresh_period, m_parameters.size(),
                    m_alert_enabled ? "enabled" : "disabled");
        render_and_send();
    }

    void process_event(const data::event& event, done_callback done) override {
        if (!m_screen || event.source != "system") {
            done();
            return;
        }

        const bool alert =
            m_alert_enabled &&
            event.priority != static_cast<int>(data::priority::ok);
        if (event.name == "cpu") {
            m_screen->set_cpu(event.message, alert);
        } else if (event.name == "temperature") {
            m_screen->set_temp(event.message, alert);
        }
        done();
    }

    void render_and_send() {
        std::lock_guard<std::mutex> render_lock(m_render_mutex);
        if (!m_client || !m_screen) {
            return;
        }

        const int width = m_client->valid()
                              ? static_cast<int>(m_client->get_width())
                              : k_default_width;
        const int height = m_client->valid()
                               ? static_cast<int>(m_client->get_height())
                               : k_default_height;
        cv::Mat image(height, width, CV_8UC1, cv::Scalar(0));

        m_screen->set_hostname(get_hostname());
        m_screen->set_network(get_network_address(m_network_interfaces));
        m_screen->render(image);
        cv::threshold(image, image, 127, 255, cv::THRESH_BINARY);

        sensor_msgs::msg::Image msg;
        msg.header.stamp =
            node_context()->get_node_clock_interface()->get_clock()->now();
        msg.header.frame_id = "display";
        msg.height = static_cast<uint32_t>(image.rows);
        msg.width = static_cast<uint32_t>(image.cols);
        msg.encoding = sensor_msgs::image_encodings::MONO8;
        msg.is_bigendian = false;
        msg.step = static_cast<uint32_t>(image.step[0]);
        msg.data.assign(image.datastart, image.dataend);

        try {
            m_client->send_image(msg);
        } catch (const std::exception& e) {
            RCLCPP_ERROR(m_logger, "Failed to send display image: %s",
                         e.what());
        }
    }

    std::string m_base_path{"display"};
    double m_refresh_period{1.0};
    std::vector<std::string> m_parameters{"hostname", "network", "cpu", "temp"};
    std::vector<std::string> m_network_interfaces{"wlan0", "eth0"};
    display_ui::font m_font;
    bool m_alert_enabled{true};

    std::unique_ptr<display_screen> m_screen;
    std::shared_ptr<clover2_display::client> m_client;
    rclcpp::TimerBase::SharedPtr m_refresh_timer;
    std::mutex m_render_mutex;
    rclcpp::Logger m_logger{rclcpp::get_logger("notification_display_output")};
};

}  // namespace clover2_notification::outputs

PLUGINLIB_EXPORT_CLASS(clover2_notification::outputs::display,
                       clover2_notification::output)
