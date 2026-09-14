/**
 * @file layout.hpp
 * @brief Common owning base class for display UI layouts.
 */

#pragma once

#include <clover2_notification/display_ui/widget.hpp>

#include <memory>
#include <vector>

namespace clover2_notification::outputs::display_ui {

class layout : public widget {
public:
    void add(std::unique_ptr<widget> child);

protected:
    std::vector<std::unique_ptr<widget>> m_children;
};

}  // namespace clover2_notification::outputs::display_ui
