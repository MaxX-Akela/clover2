#include <clover2_notification/display_ui/label.hpp>

#include <algorithm>
#include <utility>

namespace clover2_notification::outputs::display_ui {

label::label(std::string text, const font style)
    : m_text(std::move(text))
    , m_font(style) {}

void label::set_text(std::string text) { m_text = std::move(text); }

const std::string& label::text() const { return m_text; }

void label::set_alignment(const alignment value) { m_alignment = value; }

size label::measure() const {
    int baseline{};
    const auto text_size = cv::getTextSize(m_text, m_font.face, m_font.scale,
                                           m_font.thickness, &baseline);
    return {text_size.width, text_size.height + baseline};
}

void label::render(cv::Mat& image) const {
    const int x0 = std::clamp(m_geometry.x, 0, image.cols);
    const int y0 = std::clamp(m_geometry.y, 0, image.rows);
    const int x1 = std::clamp(m_geometry.x + m_geometry.width, 0, image.cols);
    const int y1 = std::clamp(m_geometry.y + m_geometry.height, 0, image.rows);
    if (x1 <= x0 || y1 <= y0 || m_text.empty()) {
        return;
    }

    int baseline{};
    const auto text_size = cv::getTextSize(m_text, m_font.face, m_font.scale,
                                           m_font.thickness, &baseline);
    int x{};
    switch (m_alignment.horizontal) {
        case horizontal_alignment::left:
            x = 0;
            break;
        case horizontal_alignment::center:
            x = (x1 - x0 - text_size.width) / 2;
            break;
        case horizontal_alignment::right:
            x = x1 - x0 - text_size.width;
            break;
    }

    int y{};
    switch (m_alignment.vertical) {
        case vertical_alignment::top:
            y = text_size.height;
            break;
        case vertical_alignment::center:
            y = ((y1 - y0) - (text_size.height + baseline)) / 2 +
                text_size.height;
            break;
        case vertical_alignment::bottom:
            y = y1 - y0 - baseline;
            break;
    }

    cv::Mat area = image(cv::Rect(x0, y0, x1 - x0, y1 - y0));
    cv::putText(area, m_text, cv::Point(x, y), m_font.face, m_font.scale,
                cv::Scalar(255), m_font.thickness, cv::LINE_8);
}

}  // namespace clover2_notification::outputs::display_ui
