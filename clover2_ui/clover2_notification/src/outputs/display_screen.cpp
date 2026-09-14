#include <clover2_notification/display_screen.hpp>
#include <clover2_notification/display_ui/hbox_layout.hpp>
#include <opencv2/core.hpp>

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace clover2_notification::outputs {

display_screen::display_screen(display_ui::font font,
                               std::vector<std::string> parameters)
    : m_root(std::make_unique<display_ui::vbox_layout>()) {
    if (parameters.empty()) {
        throw std::invalid_argument("Display parameters should not be empty");
    }

    std::unordered_set<std::string> names;
    for (const auto& parameter : parameters) {
        if (parameter.empty() || !names.insert(parameter).second) {
            throw std::invalid_argument(
                "Display parameters should be non-empty and unique");
        }

        auto line = std::make_unique<display_ui::hbox_layout>();
        if (parameter != "hostname" && parameter != "network") {
            line->add(
                std::make_unique<display_ui::label>(parameter + ": ", font));
        }

        auto value = std::make_unique<display_ui::label>("n/a", font);
        m_value_labels.emplace(parameter, value.get());
        line->add(std::move(value));
        m_root->add(std::move(line));
    }
}

void display_screen::set_hostname(std::string value) {
    set_value("hostname", std::move(value));
}

void display_screen::set_network(std::string value) {
    set_value("network", std::move(value));
}

void display_screen::set_cpu(std::string value, const bool alert) {
    set_value("cpu", std::move(value));
    set_alert("cpu", alert);
}

void display_screen::set_temperature(std::string value, const bool alert) {
    set_value("temperature", std::move(value));
    set_alert("temperature", alert);
}

void display_screen::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [name, label] : m_value_labels) {
        (void)name;
        label->set_text("n/a");
    }
    m_alert_parameters.clear();
    m_invert_phase = false;
}

void display_screen::render(cv::Mat& image) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_root->set_geometry({0, 0, image.cols, image.rows});
    m_root->render(image);

    if (m_alert_parameters.empty()) {
        m_invert_phase = false;
        return;
    }

    m_invert_phase = !m_invert_phase;
    if (m_invert_phase) {
        cv::bitwise_not(image, image);
    }
}

void display_screen::set_value(const std::string& parameter,
                               std::string value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_value_labels.find(parameter);
    if (it != m_value_labels.end()) {
        it->second->set_text(std::move(value));
    }
}

void display_screen::set_alert(const std::string& parameter,
                               const bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_value_labels.contains(parameter)) {
        return;
    }
    if (enabled) {
        m_alert_parameters.insert(parameter);
    } else {
        m_alert_parameters.erase(parameter);
    }
}

}  // namespace clover2_notification::outputs
