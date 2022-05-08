#include "csdtitlebar.h"
#include "csdtitlebarbutton.h"

#if defined(Q_OS_WIN)
#include "qregistrywatcher.h"
#include "qtwinbackports.h"
#include <Windows.h>
#include <dwmapi.h>
#endif

#include <QApplication>
#include <QBoxLayout>
#include <QEvent>
#include <QMainWindow>
#include <QMenuBar>
#include <QPainter>
#include <QStyleOption>
#include <QTimer>
#include <QMouseEvent>

namespace CSD {

TitleBar::TitleBar(CaptionButtonStyle captionButtonStyle,
                   const QIcon &captionIcon,
                   QWidget *parent)
    : QWidget(parent), m_captionButtonStyle(captionButtonStyle) {
    this->setObjectName("TitleBar");
    int headerHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);
    int headerButtonSize = style()->pixelMetric(QStyle::PM_TitleBarButtonSize);
    this->setMinimumSize(QSize(0, headerHeight)); // was 30
    this->setMaximumSize(QSize(QWIDGETSIZE_MAX, headerHeight)); // was 30
#if defined(Q_OS_WIN)
    auto maybeColor = this->readDWMColorizationColor();
    if (maybeColor.has_value()) {
        this->m_activeColor = *maybeColor;
    }
    auto maybeWatcher = QRegistryWatcher::create(
        HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\DWM", this);
    if (maybeWatcher.has_value()) {
        this->m_watcher = *maybeWatcher;
        connect(
            this->m_watcher,
            &QRegistryWatcher::valueChanged,
            this,
            [this]() {
                auto maybeColor1 = this->readDWMColorizationColor();
                if (maybeColor1.has_value() && !this->m_activeColorOverridden) {
                    this->m_activeColor = *maybeColor1;
                    this->update();
                }
            },
            Qt::QueuedConnection);
    }
#endif
    int spacing = style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    this->m_horizontalLayout = new QHBoxLayout(this);
    this->m_horizontalLayout->setSpacing(spacing); // was 0
    this->m_horizontalLayout->setObjectName("HorizontalLayout");
    this->m_horizontalLayout->setContentsMargins(0, 0, 0, 0);

    this->m_leftMargin = new QWidget(this);
    this->m_leftMargin->setObjectName("LeftMargin");
    this->m_leftMargin->setMinimumSize(QSize(5, 0));
    this->m_leftMargin->setMaximumSize(QSize(5, QWIDGETSIZE_MAX));
    this->m_horizontalLayout->addWidget(this->m_leftMargin);

    this->m_buttonCaptionIcon =
        new TitleBarButton(TitleBarButton::CaptionIcon, this);
    this->m_buttonCaptionIcon->setObjectName("ButtonCaptionIcon");
    this->m_buttonCaptionIcon->setMinimumSize(QSize(headerButtonSize, headerButtonSize)); //was 30x30
    this->m_buttonCaptionIcon->setMaximumSize(QSize(headerButtonSize, headerButtonSize));
    this->m_buttonCaptionIcon->setFocusPolicy(Qt::NoFocus);
#if defined(Q_OS_WIN)
    int icon_size = ::GetSystemMetrics(SM_CXSMICON);
#else
    int icon_size = style()->pixelMetric(QStyle::PM_TitleBarButtonIconSize); //was 16;
#endif
    this->m_buttonCaptionIcon->setIconSize(QSize(icon_size, icon_size));
    const auto icon = [&captionIcon, this]() -> QIcon {
        if (!captionIcon.isNull()) {
            return captionIcon;
        }
        auto globalWindowIcon = this->window()->windowIcon();
        if (!globalWindowIcon.isNull()) {
            return globalWindowIcon;
        }
        globalWindowIcon = QApplication::windowIcon();
        if (!globalWindowIcon.isNull()) {
            return globalWindowIcon;
        }
#if defined(Q_OS_WIN)
        // Use system default application icon which doesn't need margin
        this->m_horizontalLayout->takeAt(
            this->m_horizontalLayout->indexOf(this->m_leftMargin));
        this->m_leftMargin->setParent(nullptr);
        HICON winIcon = ::LoadIconW(nullptr, IDI_APPLICATION);
        globalWindowIcon.addPixmap(
            QtWinBackports::qt_pixmapFromWinHICON(winIcon));
#else
#if !defined(Q_OS_DARWIN)
        if (globalWindowIcon.isNull()) {
            globalWindowIcon = QIcon::fromTheme("application-x-executable");
        }
#endif
#endif
        return globalWindowIcon;
    }();
    this->m_buttonCaptionIcon->setIcon(icon);
    this->m_horizontalLayout->addWidget(this->m_buttonCaptionIcon);

    auto *mainWindow = qobject_cast<QMainWindow *>(this->window());
    if (mainWindow != nullptr) {
        this->m_menuBar = mainWindow->menuBar();
        this->m_horizontalLayout->addWidget(this->m_menuBar);
        this->m_menuBar->setFixedHeight(headerHeight); //was 30
    }

    auto *emptySpace = new QWidget(this);
    emptySpace->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->m_horizontalLayout->addWidget(emptySpace, 1);
    int headerIconSize = style()->pixelMetric(QStyle::PM_TitleBarButtonIconSize);
    this->m_buttonMinimize =
        new TitleBarButton(TitleBarButton::Minimize, this);
    this->m_buttonMinimize->setObjectName("ButtonMinimize");
    this->m_buttonMinimize->setMinimumSize(QSize(headerButtonSize, headerButtonSize));
    this->m_buttonMinimize->setMaximumSize(QSize(headerButtonSize, headerButtonSize));
    this->m_buttonMinimize->setFocusPolicy(Qt::NoFocus);
    this->m_buttonMinimize->setIconSize(QSize(headerIconSize, headerIconSize));
    this->m_horizontalLayout->addWidget(this->m_buttonMinimize);
    connect(this->m_buttonMinimize, &QPushButton::clicked, this, [this]() {
        emit this->minimizeClicked();
    });

    this->m_buttonMaximizeRestore =
        new TitleBarButton(TitleBarButton::MaximizeRestore, this);
    this->m_buttonMaximizeRestore->setObjectName("ButtonMaximizeRestore");
    this->m_buttonMaximizeRestore->setMinimumSize(
        QSize(headerButtonSize, headerButtonSize));
    this->m_buttonMaximizeRestore->setMaximumSize(
        QSize(headerButtonSize, headerButtonSize));
    this->m_buttonMaximizeRestore->setFocusPolicy(Qt::NoFocus);
    this->m_buttonMaximizeRestore->setIconSize(QSize(headerIconSize, headerIconSize));
    this->m_horizontalLayout->addWidget(this->m_buttonMaximizeRestore);
    connect(this->m_buttonMaximizeRestore,
            &QPushButton::clicked,
            this,
            [this]() { emit this->maximizeRestoreClicked(); });

    this->m_buttonClose = new TitleBarButton(TitleBarButton::Close, this);
    this->m_buttonClose->setObjectName("ButtonClose");
    this->m_buttonClose->setMinimumSize(QSize(headerButtonSize, headerIconSize));
    this->m_buttonClose->setMaximumSize(QSize(headerButtonSize, headerIconSize));
    this->m_buttonClose->setFocusPolicy(Qt::NoFocus);
    this->m_buttonClose->setIconSize(QSize(headerIconSize, headerIconSize));
    this->m_horizontalLayout->addWidget(this->m_buttonClose);
    connect(this->m_buttonClose, &QPushButton::clicked, this, [this]() {
        emit this->closeClicked();
    });

    this->setAutoFillBackground(true);
    this->setActive(this->window()->isActiveWindow());
    this->setMaximized(static_cast<bool>(this->window()->windowState() &
                                         Qt::WindowMaximized));
}

#if defined(Q_OS_WIN)
std::optional<QColor> TitleBar::readDWMColorizationColor() {
    auto handleKey = ::HKEY();
    auto regOpenResult = ::RegOpenKeyExW(HKEY_CURRENT_USER,
                                         L"SOFTWARE\\Microsoft\\Windows\\DWM",
                                         0,
                                         KEY_READ,
                                         &handleKey);
    if (regOpenResult != ERROR_SUCCESS) {
        return std::nullopt;
    }
    auto value = ::DWORD();
    auto dwordBufferSize = ::DWORD(sizeof(::DWORD));
    auto regQueryResult = ::RegQueryValueExW(handleKey,
                                             L"ColorizationColor",
                                             nullptr,
                                             nullptr,
                                             reinterpret_cast<LPBYTE>(&value),
                                             &dwordBufferSize);
    if (regQueryResult != ERROR_SUCCESS) {
        return std::nullopt;
    }
    return QColor(static_cast<QRgb>(value));
}
#endif

TitleBar::~TitleBar() {
    auto *mainWindow = qobject_cast<QMainWindow *>(this->window());
    if (mainWindow != nullptr) {
        mainWindow->setMenuBar(this->m_menuBar);
    }
    this->m_menuBar = nullptr;
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        m_dragPosition = event->globalPos() - window()->pos();
#else
        m_dragPosition = event->globalPosition().toPoint() - window()->pos();
#endif
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        window()->move(event->globalPos() - m_dragPosition);
#else
        window()->move(event->globalPosition().toPoint() - m_dragPosition);
#endif
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void TitleBar::paintEvent([[maybe_unused]] QPaintEvent *event) {
    auto styleOption = QStyleOption();
    styleOption.initFrom(this);
    auto painter = QPainter(this);
    this->style()->drawPrimitive(
        QStyle::PE_Widget, &styleOption, &painter, this);
}

bool TitleBar::isActive() const {
    return this->m_active;
}

void TitleBar::setActive(bool active) {
    this->m_active = active;
    if (active) {
        auto palette = this->palette();
        palette.setColor(QPalette::Window, this->m_activeColor);
        this->setPalette(palette);
    } else {
        auto palette = this->palette();
        palette.setColor(QPalette::Window, this->m_inactiveColor);
        this->setPalette(palette);
    }

    auto iconsPaths =
        Internal::captionIconPathsForState(this->m_active,
                                           this->m_maximized,
                                           false,
                                           false,
                                           this->m_captionButtonStyle);

    this->m_buttonMinimize->setIcon(QIcon(iconsPaths[0].toString()));
    this->m_buttonMaximizeRestore->setIcon(QIcon(iconsPaths[1].toString()));
    this->m_buttonClose->setIcon(QIcon(iconsPaths[2].toString()));
}

bool TitleBar::isMaximized() const {
    return this->m_maximized;
}

void TitleBar::setMaximized(bool maximized) {
    this->m_maximized = maximized;
    auto iconsPaths =
        Internal::captionIconPathsForState(this->m_active,
                                           this->m_maximized,
                                           false,
                                           false,
                                           this->m_captionButtonStyle);

    this->m_buttonMinimize->setIcon(QIcon(iconsPaths[0].toString()));
    this->m_buttonMaximizeRestore->setIcon(QIcon(iconsPaths[1].toString()));
    this->m_buttonClose->setIcon(QIcon(iconsPaths[2].toString()));
}

void TitleBar::setMinimizable(bool on) {
    this->m_buttonMinimize->setVisible(on);
}

void TitleBar::setMaximizable(bool on) {
    this->m_buttonMaximizeRestore->setVisible(on);
}

QColor TitleBar::activeColor() {
    return this->m_activeColor;
}

void TitleBar::setActiveColor(const QColor &inactiveColor) {
#if defined(Q_OS_WIN)
    this->m_activeColorOverridden = true;
#endif
    this->m_activeColor = inactiveColor;
    this->update();
}

QColor TitleBar::inactiveColor() {
    return this->m_inactiveColor;
}

void TitleBar::setInactiveColor(const QColor &inactiveColor) {
    this->m_activeColor = inactiveColor;
    this->update();
}

QColor TitleBar::hoverColor() const {
    return this->m_hoverColor;
}

void TitleBar::setHoverColor(QColor hoverColor) {
    this->m_hoverColor = std::move(hoverColor);
    this->m_buttonMinimize->setHoverColor(this->m_hoverColor);
    this->m_buttonMaximizeRestore->setHoverColor(this->m_hoverColor);
}

CaptionButtonStyle TitleBar::captionButtonStyle() const {
    return this->m_captionButtonStyle;
}

void TitleBar::setCaptionButtonStyle(CaptionButtonStyle captionButtonStyle) {
    this->m_captionButtonStyle = captionButtonStyle;
    
    int pm_icon_size = style()->pixelMetric(QStyle::PM_TitleBarButtonIconSize);
    auto iconSize = QSize(pm_icon_size, pm_icon_size);
    int requiredWidth = style()->pixelMetric(QStyle::PM_TitleBarButtonSize);
    this->m_buttonMinimize->setIconSize(iconSize);
    this->m_buttonMinimize->setMinimumWidth(requiredWidth);
    this->m_buttonMinimize->setMaximumWidth(requiredWidth);
    this->m_buttonMaximizeRestore->setIconSize(iconSize);
    this->m_buttonMaximizeRestore->setMinimumWidth(requiredWidth);
    this->m_buttonMaximizeRestore->setMaximumWidth(requiredWidth);
    this->m_buttonClose->setIconSize(iconSize);
    this->m_buttonClose->setMinimumWidth(requiredWidth);
    this->m_buttonClose->setMaximumWidth(requiredWidth);

    auto iconsPaths =
        Internal::captionIconPathsForState(this->m_active,
                                           this->m_maximized,
                                           false,
                                           false,
                                           this->m_captionButtonStyle);

    this->m_buttonMinimize->setIcon(QIcon(iconsPaths[0].toString()));
    this->m_buttonMaximizeRestore->setIcon(QIcon(iconsPaths[1].toString()));
    this->m_buttonClose->setIcon(QIcon(iconsPaths[2].toString()));
}

void TitleBar::onWindowStateChange(Qt::WindowStates state) {
    this->setActive(this->window()->isActiveWindow());
    this->setMaximized(static_cast<bool>(state & Qt::WindowMaximized));
}

bool TitleBar::hovered() const {
    auto cursorPos = QCursor::pos();
    bool hovered = this->rect().contains(this->mapFromGlobal(cursorPos));
    if (!hovered) {
        return false;
    }

    if (this->m_menuBar->rect().contains(
            this->m_menuBar->mapFromGlobal(cursorPos))) {
        return false;
    }

    for (const TitleBarButton *btn : this->findChildren<TitleBarButton *>()) {
        bool btnHovered = btn->rect().contains(btn->mapFromGlobal(cursorPos));
        if (btnHovered) {
            return false;
        }
    }
    return true;
}

bool TitleBar::isCaptionButtonHovered() const {
    return this->m_buttonMinimize->underMouse() ||
           this->m_buttonMaximizeRestore->underMouse() ||
           this->m_buttonClose->underMouse();
}

void TitleBar::triggerCaptionRepaint() {
    this->m_buttonMinimize->update();
    this->m_buttonMaximizeRestore->update();
    this->m_buttonClose->update();
}

namespace Internal {

std::array<QStringView, 3> captionIconPathsForState(bool active,
                                                    bool maximized,
                                                    bool hovered,
                                                    bool pressed,
                                                    CaptionButtonStyle style) {
    std::array<QStringView, 3> buf;

    switch (style) {
    case CaptionButtonStyle::custom: {
        if (active || hovered) {
            buf[0] = u":/resources/titlebar/custom/chrome-minimize-dark.svg";
            if (maximized) {
                buf[1] =
                    u":/resources/titlebar/custom/chrome-restore-dark.svg";
            } else {
                buf[1] =
                    u":/resources/titlebar/custom/chrome-maximize-dark.svg";
            }
            if (hovered) {
                buf[2] = u":/resources/titlebar/custom/chrome-close-light.svg";
            } else {
                buf[2] = u":/resources/titlebar/custom/chrome-close-dark.svg";
            }
        } else {
            buf[0] = u":/resources/titlebar/custom/"
                     u"chrome-minimize-dark-disabled.svg";
            if (maximized) {
                buf[1] = u":/resources/titlebar/custom/"
                         u"chrome-restore-dark-disabled.svg";
            } else {
                buf[1] = u":/resources/titlebar/custom/"
                         u"chrome-maximize-dark-disabled.svg";
            }
            buf[2] =
                u":/resources/titlebar/custom/chrome-close-dark-disabled.svg";
        }
        break;
    }
    case CaptionButtonStyle::win: {
        if (active || hovered) {
            buf[0] = u":/resources/titlebar/win/chrome-minimize-dark.svg";
            if (maximized) {
                buf[1] = u":/resources/titlebar/win/chrome-restore-dark.svg";
            } else {
                buf[1] = u":/resources/titlebar/win/chrome-maximize-dark.svg";
            }
            if (hovered) {
                buf[2] = u":/resources/titlebar/win/chrome-close-light.svg";
            } else {
                buf[2] = u":/resources/titlebar/win/chrome-close-dark.svg";
            }
        } else {
            buf[0] =
                u":/resources/titlebar/win/chrome-minimize-dark-disabled.svg";
            if (maximized) {
                buf[1] = u":/resources/titlebar/win/"
                         u"chrome-restore-dark-disabled.svg";
            } else {
                buf[1] = u":/resources/titlebar/win/"
                         u"chrome-maximize-dark-disabled.svg";
            }
            buf[2] =
                u":/resources/titlebar/win/chrome-close-dark-disabled.svg";
        }
        break;
    }
    case CaptionButtonStyle::mac: {
        if (pressed) {
            buf[0] = u":/resources/titlebar/mac/minimize-pressed.png";
            if (maximized) {
                buf[1] = u":/resources/titlebar/mac/"
                         "maximize-restore-maximized-pressed.png";
            } else {
                buf[1] = u":/resources/titlebar/mac/"
                         u"maximize-restore-normal-pressed.png";
            }
            buf[2] = u":/resources/titlebar/mac/close-pressed.png";
        } else {
            if (hovered) {
                buf[0] = u":/resources/titlebar/mac/minimize-hovered.png";
                if (maximized) {
                    buf[1] = u":/resources/titlebar/mac/"
                             u"maximize-restore-maximized-hovered.png";
                } else {
                    buf[1] = u":/resources/titlebar/mac/"
                             u"maximize-restore-normal-hovered.png";
                }
                buf[2] = u":/resources/titlebar/mac/close-hovered.png";
            } else {
                if (active) {
                    buf[0] = u":/resources/titlebar/mac/minimize.png";
                    buf[1] = u":/resources/titlebar/mac/maximize-restore.png";
                    buf[2] = u":/resources/titlebar/mac/close.png";
                } else {
                    buf[0] = u":/resources/titlebar/mac/inactive.png";
                    buf[1] = u":/resources/titlebar/mac/inactive.png";
                    buf[2] = u":/resources/titlebar/mac/inactive.png";
                }
            }
        }
        break;
    }
    }

    return buf;
}

} // namespace Internal

} // namespace CSD
