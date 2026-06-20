#pragma once
#include <QSystemTrayIcon>

class QAction;

class SystemTray : public QSystemTrayIcon {
    Q_OBJECT
public:
    // enableCenterOnMe=false hides the "Center on Me" action (used by non-Earth
    // bodies, which never wire up the centerOnMe signal). Defaults true so the
    // Earth app (SystemTray tray;) keeps its existing behavior unchanged.
    explicit SystemTray(bool enableCenterOnMe = true, QObject *parent = nullptr);
    void setSolarTooltip(const QString &text);
signals:
    void toggleVisibility();
    void resetView();
    void centerOnMe();
    void openSettings();
    void quit();
};
