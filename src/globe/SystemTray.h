#pragma once
#include <QSystemTrayIcon>

class QAction;

class SystemTray : public QSystemTrayIcon {
    Q_OBJECT
public:
    explicit SystemTray(QObject *parent = nullptr);
    void setSolarTooltip(const QString &text);
    void setShowGridChecked(bool on);
signals:
    void toggleVisibility();
    void resetView();
    void centerOnMe();
    void toggleShowGrid(bool on);
    void quit();

private:
    QAction *m_showGridAction = nullptr;
};
