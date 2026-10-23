#include <windows.h>

#include <QApplication>
#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include <QSize>
#include <QRect>

#include "core/ConfigStore.h"
#include "core/PlayerController.h"
#include "core/CoverImageProvider.h"
#include "core/AppActions.h"
#include "system/WindowHost.h"

namespace {

void enableDpiAwareness()
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        using SetCtxFn = BOOL (WINAPI *)(DPI_AWARENESS_CONTEXT);
        auto setCtx = reinterpret_cast<SetCtxFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setCtx) {
            // PER_MONITOR_AWARE_V2 = (-4), PER_MONITOR_AWARE = (-3)
            if (setCtx(reinterpret_cast<DPI_AWARENESS_CONTEXT>(-4)))
                return;
            if (setCtx(reinterpret_cast<DPI_AWARENESS_CONTEXT>(-3)))
                return;
        }
    }
    SetProcessDPIAware();
}

} // namespace

int main(int argc, char *argv[])
{
    enableDpiAwareness();

    // The wallpaper window is WS_EX_LAYERED on the WorkerW/Progman layer:
    // only the ANGLE/DirectComposition backend presents through DWM there.
    // The desktop-GL (WGL) backend creates a valid context but never shows
    // any content, so force ANGLE unless the user explicitly overrides it.
    if (qEnvironmentVariableIsEmpty("QT_OPENGL"))
        qputenv("QT_OPENGL", "angle");

    QApplication::setQuitOnLastWindowClosed(false);
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("MusicDesktop"));
    QApplication::setOrganizationName(QStringLiteral("MusicDesktop"));

    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    qRegisterMetaType<MediaSnapshot>();
    qRegisterMetaType<QVector<qreal>>("QVector<qreal>");

    ConfigStore config;
    CoverImageProvider *coverProvider = new CoverImageProvider;
    PlayerController player;
    player.init(&config, coverProvider);

    AppActions actions;
    WindowHost host(&config);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("cover"), coverProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("player"), &player);
    engine.rootContext()->setContextProperty(QStringLiteral("cfg"), &config);
    engine.rootContext()->setContextProperty(QStringLiteral("appActions"), &actions);
    engine.rootContext()->setContextProperty(
        QStringLiteral("desktopSize"),
        QSize(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)));
    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    engine.rootContext()->setContextProperty(
        QStringLiteral("workArea"),
        QRect(workArea.left, workArea.top,
              workArea.right - workArea.left,
              workArea.bottom - workArea.top));

    engine.load(QUrl(QStringLiteral("qrc:/qml/MainWindow.qml")));
    engine.load(QUrl(QStringLiteral("qrc:/qml/SettingsPanel.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    QQuickWindow *wallpaper = nullptr;
    QObject *settings = nullptr;
    for (QObject *obj : engine.rootObjects()) {
        if (obj->objectName() == QLatin1String("WallpaperWindow"))
            wallpaper = qobject_cast<QQuickWindow *>(obj);
        else if (obj->objectName() == QLatin1String("SettingsPanel"))
            settings = obj;
    }
    if (!wallpaper)
        return -1;

    host.attach(wallpaper);
    player.start();

    // ---- Tray icon ----
    QSystemTrayIcon tray(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    tray.setToolTip(QStringLiteral("MusicDesktop"));
    QMenu menu;
    QAction *settingsAct = menu.addAction(QStringLiteral("设置"));
    QAction *clickAct = menu.addAction(QStringLiteral("鼠标穿透"));
    clickAct->setCheckable(true);
    clickAct->setChecked(config.boolValue(CfgKey::WindowClickThrough));
    QAction *reattachAct = menu.addAction(QStringLiteral("重新挂载 WorkerW"));
    menu.addSeparator();
    QAction *quitAct = menu.addAction(QStringLiteral("退出"));
    tray.setContextMenu(&menu);
    tray.show();

    QObject::connect(settingsAct, &QAction::triggered, &actions,
                     &AppActions::toggleSettings);
    QObject::connect(clickAct, &QAction::toggled, &config,
                     [&config](bool checked) {
                         config.setValue(CfgKey::WindowClickThrough, checked);
                     });
    QObject::connect(&config, &ConfigStore::changed, clickAct,
                     [clickAct, &config](const QString &key) {
                         if (key == QLatin1String(CfgKey::WindowClickThrough)) {
                             QSignalBlocker blocker(clickAct);
                             clickAct->setChecked(config.boolValue(CfgKey::WindowClickThrough));
                         }
                     });
    QObject::connect(reattachAct, &QAction::triggered, &actions,
                     &AppActions::requestReattach);
    QObject::connect(quitAct, &QAction::triggered, &actions,
                     &AppActions::requestQuit);
    QObject::connect(&tray, &QSystemTrayIcon::activated, &actions,
                     [&actions](QSystemTrayIcon::ActivationReason reason) {
                         if (reason == QSystemTrayIcon::DoubleClick
                             || reason == QSystemTrayIcon::Trigger)
                             actions.toggleSettings();
                     });
    QObject::connect(&actions, &AppActions::settingsToggled, settings, [settings]() {
        if (!settings)
            return;
        const bool visible = settings->property("visible").toBool();
        settings->setProperty("visible", !visible);
        if (!visible)
            QMetaObject::invokeMethod(settings, "raise");
    });
    QObject::connect(&actions, &AppActions::reattachRequested,
                     &host, &WindowHost::reattach);
    QObject::connect(&actions, &AppActions::quitRequested, &app, &QCoreApplication::quit);

    if (!QSystemTrayIcon::isSystemTrayAvailable())
        tray.showMessage(QStringLiteral("MusicDesktop"),
                         QStringLiteral("托盘不可用，程序将在后台继续运行"));

    const int rc = app.exec();

    player.shutdown();
    config.syncNow();
    return rc;
}
