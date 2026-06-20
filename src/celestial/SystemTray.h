#pragma once
#include <QSystemTrayIcon>

class QAction;

class SystemTray : public QSystemTrayIcon {
    Q_OBJECT
public:
    explicit SystemTray(QObject *parent = nullptr);
    void setSolarTooltip(const QString &text);
    void setFlatMapChecked(bool checked);
signals:
    void toggleVisibility();
    void resetView();
    void centerOnMe();
    void flatMapToggled(bool flat);
    void openSettings();
    void quit();
private:
    QAction *m_flatMapAction = nullptr;
};
