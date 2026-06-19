#include "SystemTray.h"
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QPixmap>

SystemTray::SystemTray(QObject *parent) : QSystemTrayIcon(parent) {
    QPixmap pm(16, 16);
    pm.fill(QColor(40, 96, 200));
    setIcon(QIcon(pm));

    auto *menu = new QMenu;
    menu->addAction(tr("Hide/Show"), this, &SystemTray::toggleVisibility);
    menu->addAction(tr("Reset View"), this, &SystemTray::resetView);
    menu->addAction(tr("Center on Me"), this, &SystemTray::centerOnMe);
    menu->addSeparator();
    menu->addAction(tr("Settings..."), this, &SystemTray::openSettings);
    menu->addSeparator();
    menu->addAction(tr("About"), this, [] {
        // v1: About is a tray message. A real dialog can replace this later.
    });
    auto *quit = menu->addAction(tr("Quit"));
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);

    setContextMenu(menu);
    show();
}

void SystemTray::setSolarTooltip(const QString &text) {
    setToolTip(text);
}

