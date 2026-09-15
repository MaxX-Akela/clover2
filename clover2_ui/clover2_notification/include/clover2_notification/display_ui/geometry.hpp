/**
 * @file geometry.hpp
 * @brief Geometry and alignment types for display UI widgets.
 */

#pragma once

namespace clover2_notification::outputs::display_ui {

struct size {
    int width{};
    int height{};
};

struct rect {
    int x{};
    int y{};
    int width{};
    int height{};
};

enum class horizontal_alignment { left, center, right };
enum class vertical_alignment { top, center, bottom };

struct alignment {
    horizontal_alignment horizontal{horizontal_alignment::left};
    vertical_alignment vertical{vertical_alignment::center};
};

}  // namespace clover2_notification::outputs::display_ui
