#include <clover2_notification/display_ui/vbox_layout.hpp>

#include <algorithm>

namespace clover2_notification::outputs::display_ui {

size vbox_layout::measure() const {
    size result;
    for (const auto& child : m_children) {
        const auto child_size = child->measure();
        result.width = std::max(result.width, child_size.width);
        result.height += child_size.height;
    }
    return result;
}

void vbox_layout::set_geometry(const rect geometry) {
    widget::set_geometry(geometry);
    if (m_children.empty()) {
        return;
    }

    const int count = static_cast<int>(m_children.size());
    const int base_height = std::max(0, geometry.height) / count;
    int remainder = std::max(0, geometry.height) % count;
    int y = geometry.y;
    for (const auto& child : m_children) {
        const int height = base_height + (remainder-- > 0 ? 1 : 0);
        child->set_geometry(
            {geometry.x, y, std::max(0, geometry.width), height});
        y += height;
    }
}

void vbox_layout::render(cv::Mat& image) const {
    for (const auto& child : m_children) {
        child->render(image);
    }
}

}  // namespace clover2_notification::outputs::display_ui
