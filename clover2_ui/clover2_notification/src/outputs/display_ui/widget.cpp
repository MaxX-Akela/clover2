#include <clover2_notification/display_ui/widget.hpp>

namespace clover2_notification::outputs::display_ui {

void widget::set_geometry(const rect geometry) { m_geometry = geometry; }

const rect& widget::geometry() const { return m_geometry; }

}  // namespace clover2_notification::outputs::display_ui
