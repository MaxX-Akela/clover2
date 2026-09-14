/**
 * @file display_screen.hpp
 * @brief Notification status screen built from display UI widgets.
 */

#pragma once

#include <clover2_notification/display_ui/label.hpp>
#include <clover2_notification/display_ui/vbox_layout.hpp>
#include <opencv2/core.hpp>

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace clover2_notification::outputs {

class display_screen final {
public:
    display_screen(display_ui::font font, std::vector<std::string> parameters);

    void set_hostname(std::string value);
    void set_network(std::string value);
    void set_cpu(std::string value, bool alert);
    void set_temp(std::string value, bool alert);
    void clear();
    void render(cv::Mat& image);

private:
    void set_value(const std::string& parameter, std::string value);
    void set_alert(const std::string& parameter, bool enabled);

    std::unique_ptr<display_ui::vbox_layout> m_root;
    std::unordered_map<std::string, display_ui::label*> m_value_labels;
    std::unordered_set<std::string> m_alert_parameters;
    bool m_invert_phase{false};
    std::mutex m_mutex;
};

}  // namespace clover2_notification::outputs
