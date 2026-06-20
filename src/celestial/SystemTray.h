#pragma once
#include <QSystemTrayIcon>

class QMenu;

class SystemTray : public QSystemTrayIcon {
    Q_OBJECT
public:
    explicit SystemTray(QObject *parent = nullptr);
    void setSolarTooltip(const QString &text);
    // Rebuild the context menu so its labels pick up the current language.
    void retranslateMenu();
signals:
    void toggleVisibility();
    void resetView();
    void centerOnMe();
    void openSettings();
    void quit();
private:
    void buildMenu();
    QMenu *m_menu = nullptr;
};
