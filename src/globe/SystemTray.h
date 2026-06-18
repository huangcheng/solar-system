#pragma once
#include <QSystemTrayIcon>

class SystemTray : public QSystemTrayIcon {
    Q_OBJECT
public:
    explicit SystemTray(QObject *parent = nullptr);
    void setSolarTooltip(const QString &text);
signals:
    void toggleVisibility();
    void resetView();
    void quit();
};
