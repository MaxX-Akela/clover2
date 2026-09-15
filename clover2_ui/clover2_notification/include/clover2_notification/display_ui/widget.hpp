/**
 * @file widget.hpp
 * @brief Base display UI widget.
 */

#pragma once

#include <clover2_notification/display_ui/geometry.hpp>

namespace cv {
class Mat;
}

namespace clover2_notification::outputs::display_ui {

class widget {
public:
    virtual ~widget() = default;

    virtual size measure() const = 0;
    virtual void set_geometry(rect geometry);
    virtual void render(cv::Mat& image) const = 0;

    const rect& geometry() const;

protected:
    rect m_geometry;
};

}  // namespace clover2_notification::outputs::display_ui
