#include "SettingsDialog.h"
#include "ConfigManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(ConfigManager *config, QWidget *parent)
    : QDialog(parent), m_config(config) {
    setWindowTitle(tr("Settings"));
    setupUi();
}

void SettingsDialog::setupUi() {
    auto *layout = new QVBoxLayout(this);

    auto *form = new QFormLayout;

    m_languageCombo = new QComboBox;
    m_languageCombo->addItem(tr("English"), QStringLiteral("en"));
    m_languageCombo->addItem(QStringLiteral(u"\u7b80\u4f53\u4e2d\u6587"), QStringLiteral("zh_CN"));
    const int langIndex = m_languageCombo->findData(m_config->language());
    m_languageCombo->setCurrentIndex(langIndex >= 0 ? langIndex : 0);
    form->addRow(tr("Language:"), m_languageCombo);

    m_gridCheck = new QCheckBox(tr("Show Grid"));
    m_gridCheck->setChecked(m_config->showGrid());
    form->addRow(m_gridCheck);

    m_alwaysOnTopCheck = new QCheckBox(tr("Always on Top"));
    m_alwaysOnTopCheck->setChecked(m_config->alwaysOnTop());
    form->addRow(m_alwaysOnTopCheck);

    auto *nightGroup = new QGroupBox(tr("Night Mode"));
    auto *nightLayout = new QVBoxLayout(nightGroup);
    m_simpleNightRadio = new QRadioButton(tr("Simple Night"));
    m_textureNightRadio = new QRadioButton(tr("Realistic Night (city lights)"));
    nightLayout->addWidget(m_simpleNightRadio);
    nightLayout->addWidget(m_textureNightRadio);
    if (m_config->nightMode() == QStringLiteral("texture"))
        m_textureNightRadio->setChecked(true);
    else
        m_simpleNightRadio->setChecked(true);
    form->addRow(nightGroup);

    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void SettingsDialog::accept() {
    if (!m_config) { QDialog::accept(); return; }

    m_config->setLanguage(m_languageCombo->currentData().toString());
    m_config->setShowGrid(m_gridCheck->isChecked());
    m_config->setAlwaysOnTop(m_alwaysOnTopCheck->isChecked());
    m_config->setNightMode(m_textureNightRadio->isChecked()
                               ? QStringLiteral("texture") : QStringLiteral("simple"));
    m_config->save();
    emit settingsChanged();
    QDialog::accept();
}
