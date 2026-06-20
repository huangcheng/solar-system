#include "AboutDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFont>
#include <QPixmap>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("About Globe"));
    setMinimumWidth(380);
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    auto *iconLabel = new QLabel;
    QPixmap pm(QStringLiteral(":/data/icon.png"));
    if (!pm.isNull())
        iconLabel->setPixmap(pm.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    auto *nameLabel = new QLabel(QStringLiteral("Globe"));
    QFont nameFont = nameLabel->font();
    nameFont.setPointSize(16);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);
    nameLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(nameLabel);

    auto *versionLabel = new QLabel(
        QStringLiteral("Version %1 · %2")
            .arg(QStringLiteral(GLOBE_VERSION), QStringLiteral(GLOBE_GIT_HASH)));
    versionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(versionLabel);

    auto *authorLabel = new QLabel(tr("Created by HUANG Cheng"));
    authorLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(authorLabel);

    auto *descLabel = new QLabel(tr(
        "A tiny desktop Earth globe with real-time day/night, a flat-map view, "
        "and a growing solar-system mode."));
    descLabel->setWordWrap(true);
    descLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(descLabel);

    auto *creditsTitle = new QLabel(tr("Credits"));
    QFont creditsFont = creditsTitle->font();
    creditsFont.setBold(true);
    creditsTitle->setFont(creditsFont);
    layout->addWidget(creditsTitle);

    auto *creditsLabel = new QLabel(tr(
        "Textures: <a href=\"https://www.solarsystemscope.com/textures/\">Solar System Scope</a><br>"
        "City data: <a href=\"https://simplemaps.com/data/world-cities\">SimpleMaps World Cities</a><br>"
        "Built with <a href=\"https://www.qt.io/\">Qt6</a> and OpenGL"));
    creditsLabel->setWordWrap(true);
    creditsLabel->setOpenExternalLinks(true);
    layout->addWidget(creditsLabel);

    auto *licenseLabel = new QLabel(tr(
        "Licensed under the <a href=\"https://github.com/huangcheng/solar-system/blob/main/LICENSE\">MIT License</a>.<br>"
        "Source: <a href=\"https://github.com/huangcheng/solar-system\">github.com/huangcheng/solar-system</a>"));
    licenseLabel->setWordWrap(true);
    licenseLabel->setOpenExternalLinks(true);
    layout->addWidget(licenseLabel);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons);
}
