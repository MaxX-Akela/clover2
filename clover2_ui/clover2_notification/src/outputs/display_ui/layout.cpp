#include <clover2_notification/display_ui/layout.hpp>

#include <utility>

namespace clover2_notification::outputs::display_ui {

void layout::add(std::unique_ptr<widget> child) {
    m_children.push_back(std::move(child));
}

}  // namespace clover2_notification::outputs::display_ui
