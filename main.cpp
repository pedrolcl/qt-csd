#include <QApplication>
#include <QBoxLayout>
#include <QMainWindow>
#include <QPushButton>
#include <QCheckBox>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>

#include "csdtitlebar.h"
#ifdef _WIN32
#include "win32csd.h"
#else
#include "linuxcsd.h"
#endif

class DemoWindow : public QMainWindow {

public:
    DemoWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        this->setCentralWidget(new QWidget(this));
        QMenu *fileMenu = menuBar()->addMenu("&File");
        fileMenu->addSeparator();
        auto *closeAct = fileMenu->addAction("&Quit", this, &QWidget::close);
        closeAct->setStatusTip("Exit the application");
        QMenu *aboutMenu = menuBar()->addMenu("About");
        QAction *aboutAct = aboutMenu->addAction("&About", this, [this] {
            QMessageBox::about(this, "About qt-csd Demo", "Qt Client Side Decorations Demo");
        });
        aboutAct->setStatusTip(tr("Show the application's About box"));
        QAction *aboutQtAct = aboutMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
        aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
        auto *layout = new QVBoxLayout;
        layout->setMargin(0);
        this->centralWidget()->setLayout(layout);
        auto *buttonToggleFullScr = new QPushButton("Toggle full screen", this);
        connect(buttonToggleFullScr, &QPushButton::clicked, this, [this] {
            this->setWindowState(this->windowState() ^ Qt::WindowFullScreen);
        });
        auto *checkBoxMinimize = new QCheckBox("Minimizable", this);
        checkBoxMinimize->setChecked(true);
        auto *checkBoxMaximize = new QCheckBox("Maximizable", this);
        checkBoxMaximize->setChecked(true);
        auto *checkBoxResize = new QCheckBox("Resizable", this);
        checkBoxResize->setChecked(true);
        auto *subWidget = new QWidget(this);
        auto *outerLayout = new QVBoxLayout();
        outerLayout->addStretch();
        auto *centralLayout1 = new QHBoxLayout();
        centralLayout1->addStretch();
        centralLayout1->addWidget(checkBoxMaximize);
        centralLayout1->addStretch();
        outerLayout->addLayout(centralLayout1);
        auto *centralLayout2 = new QHBoxLayout();
        centralLayout2->addStretch();
        centralLayout2->addWidget(checkBoxMinimize);
        centralLayout2->addStretch();
        outerLayout->addLayout(centralLayout2);
        auto *centralLayout3 = new QHBoxLayout();
        centralLayout3->addStretch();
        centralLayout3->addWidget(checkBoxResize);
        centralLayout3->addStretch();
        outerLayout->addLayout(centralLayout3);
        auto *centralLayout4 = new QHBoxLayout();
        centralLayout4->addStretch();
        centralLayout4->addWidget(buttonToggleFullScr);
        centralLayout4->addStretch();
        outerLayout->addLayout(centralLayout4);
        outerLayout->addStretch();
        subWidget->setLayout(outerLayout);
        this->m_titleBar = new CSD::TitleBar(
#ifdef _WIN32
            CSD::CaptionButtonStyle::win,
#else
            CSD::CaptionButtonStyle::custom,
#endif
            QIcon(),
            this);
        connect(checkBoxMinimize, &QCheckBox::toggled, this, [this](bool checked) {
            this->m_titleBar->setMinimizable(checked);
        });
        connect(checkBoxMaximize, &QCheckBox::toggled, this, [this](bool checked) {
            this->m_titleBar->setMaximizable(checked);
        });
        layout->addWidget(this->m_titleBar);
        layout->addWidget(subWidget);
        this->statusBar()->showMessage("Resize me by the grip ...");
        connect(checkBoxResize, &QCheckBox::toggled, this, [this](bool checked){
            this->statusBar()->setVisible(checked);
        });
        connect(
            this->m_titleBar, &CSD::TitleBar::minimizeClicked, this, [this]() {
                this->setWindowState(this->windowState() |
                                     Qt::WindowMinimized);
            });
        connect(this->m_titleBar,
                &CSD::TitleBar::maximizeRestoreClicked,
                this,
                [this]() {
                    this->setWindowState(this->windowState() ^
                                         Qt::WindowMaximized);
                });
        connect(this->m_titleBar,
                &CSD::TitleBar::closeClicked,
                this,
                &QWidget::close);
    }

    CSD::TitleBar *titleBar() {
        return this->m_titleBar;
    }

private:
    CSD::TitleBar *m_titleBar;
};

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    auto *app = new QApplication(argc, argv);
    QApplication::setApplicationName("qt-csd");
    auto *mainWindow = new DemoWindow();
    mainWindow->resize(640, 480);

#ifdef _WIN32
    auto *filter = new CSD::Internal::Win32ClientSideDecorationFilter(app);
    app->installNativeEventFilter(filter);
#else
    auto *filter = new CSD::Internal::LinuxClientSideDecorationFilter(app);
#endif
    filter->apply(
        mainWindow,
#ifdef _WIN32
        [mainWindow]() { return mainWindow->titleBar()->hovered(); },
#endif
        [mainWindow] {
            const bool on = mainWindow->isActiveWindow();
            mainWindow->titleBar()->setActive(on);
        },
        [mainWindow] {
            mainWindow->titleBar()->onWindowStateChange(
                mainWindow->windowState());
        });

    mainWindow->show();
    return app->exec();
}
