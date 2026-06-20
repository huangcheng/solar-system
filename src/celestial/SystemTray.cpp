#include "SystemTray.h"
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QPixmap>

SystemTray::SystemTray(QObject *parent) : QSystemTrayIcon(parent) {
    setIcon(QIcon(QStringLiteral(":/data/icon.png")));
    buildMenu();
    setContextMenu(m_menu);
    show();
}

void SystemTray::buildMenu() {
    // (Re)build the whole menu from translated strings. clear() drops the old
    // actions and their connections so re-calling on a language change neither
    // duplicates items nor leaves dangling signal wiring.
    if (!m_menu) {
        m_menu = new QMenu;
    } else {
        m_menu->clear();
    }
    m_menu->addAction(tr("Hide/Show"), this, &SystemTray::toggleVisibility);
    m_menu->addAction(tr("Reset View"), this, &SystemTray::resetView);
    m_menu->addAction(tr("Center on Me"), this, &SystemTray::centerOnMe);
    m_menu->addSeparator();
    m_menu->addAction(tr("Settings..."), this, &SystemTray::openSettings);
    m_menu->addSeparator();
    m_menu->addAction(tr("About"), this, &SystemTray::openAbout);
    auto *quit = m_menu->addAction(tr("Quit"));
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);
}

void SystemTray::retranslateMenu() {
    buildMenu();
    setContextMenu(m_menu);
}

void SystemTray::setSolarTooltip(const QString &text) {
    setToolTip(text);
}
