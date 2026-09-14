#include <clover2_notification/display_ui/hbox_layout.hpp>

#include <algorithm>

namespace clover2_notification::outputs::display_ui {

size hbox_layout::measure() const {
    size result;
    for (const auto& child : m_children) {
        const auto child_size = child->measure();
        result.width += child_size.width;
        result.height = std::max(result.height, child_size.height);
    }
    return result;
}

void hbox_layout::set_geometry(const rect geometry) {
    widget::set_geometry(geometry);
    int x = geometry.x;
    const int right = geometry.x + std::max(0, geometry.width);
    for (const auto& child : m_children) {
        const int width =
            std::min(child->measure().width, std::max(0, right - x));
        child->set_geometry(
            {x, geometry.y, width, std::max(0, geometry.height)});
        x += width;
    }
}

void hbox_layout::render(cv::Mat& image) const {
    for (const auto& child : m_children) {
        child->render(image);
    }
}

}  // namespace clover2_notification::outputs::display_ui
