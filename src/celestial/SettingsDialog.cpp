#include "SettingsDialog.h"
#include "ConfigManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QWidget>
#include <QDialogButtonBox>
#include <QSlider>

SettingsDialog::SettingsDialog(ConfigManager *config, QWidget *parent)
    : QDialog(parent), m_config(config) {
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
    m_langLabel = new QLabel(tr("Language:"));
    form->addRow(m_langLabel, m_languageCombo);

    m_viewModeCombo = new QComboBox;
    m_viewModeCombo->addItem(tr("Globe (3D)"), QStringLiteral("globe"));
    m_viewModeCombo->addItem(tr("Flat Map"), QStringLiteral("map"));
    m_viewModeCombo->setCurrentIndex(qMax(0, m_viewModeCombo->findData(m_config->viewMode())));
    m_viewModeLabel = new QLabel(tr("View Mode:"));
    form->addRow(m_viewModeLabel, m_viewModeCombo);

    m_gridCheck = new QCheckBox(tr("Show Grid"));
    m_gridCheck->setChecked(m_config->showGrid());
    form->addRow(m_gridCheck);

    m_alwaysOnTopCheck = new QCheckBox(tr("Always on Top"));
    m_alwaysOnTopCheck->setChecked(m_config->alwaysOnTop());
    form->addRow(m_alwaysOnTopCheck);

    m_markerCheck = new QCheckBox(tr("Show home marker"));
    m_markerCheck->setChecked(m_config->showHomeMarker());
    form->addRow(m_markerCheck);

    // Home marker location: manual latitude / longitude (independent of geolocation).
    auto *homeRow = new QWidget;
    auto *homeLay = new QHBoxLayout(homeRow);
    homeLay->setContentsMargins(0, 0, 0, 0);
    m_latSpin = new QDoubleSpinBox;
    m_latSpin->setRange(-90.0, 90.0);
    m_latSpin->setDecimals(4);
    m_latSpin->setSuffix(tr("\xc2\xb0"));   // degree sign
    m_latSpin->setValue(m_config->homeLatitude());
    m_lonSpin = new QDoubleSpinBox;
    m_lonSpin->setRange(-180.0, 180.0);
    m_lonSpin->setDecimals(4);
    m_lonSpin->setSuffix(tr("\xc2\xb0"));
    m_lonSpin->setValue(m_config->homeLongitude());
    homeLay->addWidget(m_latSpin);
    homeLay->addWidget(m_lonSpin);
    m_homeLocLabel = new QLabel(tr("Home marker location:"));
    form->addRow(m_homeLocLabel, homeRow);

    m_autoLocCheck = new QCheckBox(tr("Auto-detect my location"));
    m_autoLocCheck->setChecked(m_config->locationOptIn());
    form->addRow(m_autoLocCheck);

    m_nightGroup = new QGroupBox(tr("Night Mode"));
    auto *nightLayout = new QVBoxLayout(m_nightGroup);
    m_simpleNightRadio = new QRadioButton(tr("Simple Night"));
    m_textureNightRadio = new QRadioButton(tr("Realistic Night (city lights)"));
    nightLayout->addWidget(m_simpleNightRadio);
    nightLayout->addWidget(m_textureNightRadio);
    if (m_config->nightMode() == QStringLiteral("texture"))
        m_textureNightRadio->setChecked(true);
    else
        m_simpleNightRadio->setChecked(true);
    form->addRow(m_nightGroup);

    // Rotation speed slider (1 = real-time, 2880 = fast toy spin).
    m_rotationSlider = new QSlider(Qt::Horizontal);
    m_rotationSlider->setRange(1, 2880);
    m_rotationSlider->setSingleStep(10);
    m_rotationSlider->setPageStep(100);
    m_rotationSlider->setValue(m_config->rotationSpeed());
    m_rotationValueLabel = new QLabel;
    auto *rotWidget = new QWidget;
    auto *rotRow = new QHBoxLayout(rotWidget);
    rotRow->setContentsMargins(0, 0, 0, 0);
    rotRow->addWidget(m_rotationSlider, 1);
    rotRow->addWidget(m_rotationValueLabel);
    m_spinLabel = new QLabel(tr("Spin Speed:"));
    form->addRow(m_spinLabel, rotWidget);
    onRotationSlider(m_rotationSlider->value());
    connect(m_rotationSlider, &QSlider::valueChanged, this, &SettingsDialog::onRotationSlider);

    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    retranslateUi();
}

void SettingsDialog::onRotationSlider(int value) {
    QString desc;
    if (value <= 1) desc = tr("Real-time");
    else if (value <= 60) desc = tr("Slow");
    else if (value <= 480) desc = tr("Medium");
    else desc = tr("Fast");
    m_rotationValueLabel->setText(QStringLiteral("%1x (%2)").arg(value).arg(desc));
}

void SettingsDialog::retranslateUi() {
    setWindowTitle(tr("Settings"));
    m_gridCheck->setText(tr("Show Grid"));
    m_alwaysOnTopCheck->setText(tr("Always on Top"));
    m_markerCheck->setText(tr("Show home marker"));
    m_autoLocCheck->setText(tr("Auto-detect my location"));
    m_langLabel->setText(tr("Language:"));
    m_viewModeLabel->setText(tr("View Mode:"));
    m_homeLocLabel->setText(tr("Home marker location:"));
    m_spinLabel->setText(tr("Spin Speed:"));
    m_simpleNightRadio->setText(tr("Simple Night"));
    m_textureNightRadio->setText(tr("Realistic Night (city lights)"));
    m_nightGroup->setTitle(tr("Night Mode"));
    // Combo item texts (re-applied in place; data values are untouched).
    m_viewModeCombo->setItemText(0, tr("Globe (3D)"));
    m_viewModeCombo->setItemText(1, tr("Flat Map"));
    m_languageCombo->setItemText(0, tr("English"));
    // Spinbox suffixes.
    m_latSpin->setSuffix(tr("\xc2\xb0"));
    m_lonSpin->setSuffix(tr("\xc2\xb0"));
    // Spin-speed description uses tr() internally.
    onRotationSlider(m_rotationSlider->value());
}

void SettingsDialog::accept() {
    if (!m_config) { QDialog::accept(); return; }

    m_config->setLanguage(m_languageCombo->currentData().toString());
    m_config->setShowGrid(m_gridCheck->isChecked());
    m_config->setAlwaysOnTop(m_alwaysOnTopCheck->isChecked());
    m_config->setRotationSpeed(m_rotationSlider->value());
    m_config->setNightMode(m_textureNightRadio->isChecked()
                               ? QStringLiteral("texture") : QStringLiteral("simple"));
    m_config->setViewMode(m_viewModeCombo->currentData().toString());
    m_config->setShowHomeMarker(m_markerCheck->isChecked());
    m_config->setLocationOptIn(m_autoLocCheck->isChecked());
    m_config->setHomeLatitude(m_latSpin->value());
    m_config->setHomeLongitude(m_lonSpin->value());
    m_config->save();
    emit settingsChanged();
    QDialog::accept();
}
