#pragma once

#include <QObject>

/** Cross-layer UI actions (tray <-> QML), lives on the main thread. */
class AppActions : public QObject
{
    Q_OBJECT
public:
    explicit AppActions(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void toggleSettings() { emit settingsToggled(); }
    Q_INVOKABLE void requestQuit() { emit quitRequested(); }
    Q_INVOKABLE void requestReattach() { emit reattachRequested(); }

signals:
    void settingsToggled();
    void quitRequested();
    void reattachRequested();
};
