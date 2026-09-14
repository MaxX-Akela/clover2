/**
 * @file hbox_layout.hpp
 * @brief Horizontal display UI layout.
 */

#pragma once

#include <clover2_notification/display_ui/layout.hpp>

namespace clover2_notification::outputs::display_ui {

class hbox_layout final : public layout {
public:
    size measure() const override;
    void set_geometry(rect geometry) override;
    void render(cv::Mat& image) const override;
};

}  // namespace clover2_notification::outputs::display_ui
