#pragma once

#include <Windows.h>
#include <dwmapi.h>
#include <windowsx.h>

#include <QAbstractNativeEventFilter>
#include <QMargins>
#include <QMetaType>
#include <QObject>

#include <functional>
#include <unordered_map>

Q_DECLARE_METATYPE(QMargins)

class QWidget;

namespace CSD::Internal {

class Win32ClientSideDecorationFilter : public QObject,
                                        public QAbstractNativeEventFilter {
    Q_OBJECT

private:
    struct HWNDData {
        QWidget *widget;
        std::function<bool()> isCaptionHovered;
        std::function<void()> onActivationChanged;
        std::function<void()> onWindowStateChanged;
        HWNDData(QWidget *widget,
                 std::function<bool()> isCaptionHovered,
                 std::function<void()> onActivationChanged,
                 std::function<void()> onWindowStateChanged);
    };
    std::unordered_map<HWND, HWNDData> appliedHWNDs;

public:
    explicit Win32ClientSideDecorationFilter(QObject *parent = nullptr);
    ~Win32ClientSideDecorationFilter() override;

    bool eventFilter(QObject *watched, QEvent *event) override;
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)	
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
#else
	bool nativeEventFilter(const QByteArray &eventType, void *message,  qintptr *result) override;
#endif	
    void apply(QWidget *widget,
               std::function<bool()> isCaptionHovered,
               std::function<void()> onActivationChanged,
               std::function<void()> onWindowStateChanged);
};
} // namespace CSD::Internal
