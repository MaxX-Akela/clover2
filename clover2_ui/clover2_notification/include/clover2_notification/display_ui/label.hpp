/**
 * @file label.hpp
 * @brief Monochrome text widget.
 */

#pragma once

#include <clover2_notification/display_ui/widget.hpp>
#include <opencv2/imgproc.hpp>

#include <string>

namespace clover2_notification::outputs::display_ui {

struct font {
    int face{cv::FONT_HERSHEY_SIMPLEX};
    double scale{0.45};
    int thickness{1};
};

class label final : public widget {
public:
    explicit label(std::string text = {}, font style = {});

    void set_text(std::string text);
    const std::string& text() const;
    void set_alignment(alignment value);

    size measure() const override;
    void render(cv::Mat& image) const override;

private:
    std::string m_text;
    font m_font;
    alignment m_alignment;
};

}  // namespace clover2_notification::outputs::display_ui
