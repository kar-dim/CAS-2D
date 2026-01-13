#pragma once

#include <concepts>
#include <QPixmap>
#include <QSize>
#include <Qt>
#include <QWidget>
#include <type_traits>

// Utility class for common widget operations
class WidgetUtils final {
  public:
    WidgetUtils() = delete;
    WidgetUtils(const WidgetUtils&) = delete;
    WidgetUtils& operator=(const WidgetUtils&) = delete;
    WidgetUtils(WidgetUtils&&) = delete;
    WidgetUtils& operator=(WidgetUtils&&) = delete;

    // scale the pixmap to the target size
    inline static void scalePixmap(QPixmap& pixmap, const QSize targetImageSize) {
        if (pixmap.size().width() > targetImageSize.width() || pixmap.size().height() > targetImageSize.height())
            pixmap = pixmap.scaled(targetImageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    // templated method to set visibility of multiple widgets
    template <typename... Widgets>
        requires(std::derived_from<std::remove_pointer_t<Widgets>, QWidget> && ...)
    static void setVisibility(bool value, Widgets... widgets) {
        (widgets->setVisible(value), ...);
    }
};
