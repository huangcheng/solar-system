#include "SettingsDialog.h"
#include "ConfigManager.h"
#include "CityDatabase.h"
#include "LocationProvider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QCompleter>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QGroupBox>
#include <QLabel>
#include <QWidget>
#include <QDialogButtonBox>
#include <QSlider>

SettingsDialog::SettingsDialog(ConfigManager *config, QWidget *parent, LocationProvider *location)
    : QDialog(parent), m_config(config), m_location(location) {
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

    // --- Home location: city selector + detect button + auto-start + coords ---

    m_cityCombo = new QComboBox;
    m_cityCombo->setEditable(true);
    m_cityCombo->setInsertPolicy(QComboBox::NoInsert);   // never keep typed garbage
    {
        const auto &cities = CityDatabase::instance().all();
        QStringList displays;
        displays.reserve(cities.size());
        for (const CityEntry &c : cities)
            displays << c.display();
        m_cityCombo->addItems(displays);   // populated before the activated signal is wired
    }
    m_cityCompleter = new QCompleter(m_cityCombo->model(), this);   // share the combo model
    m_cityCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_cityCompleter->setFilterMode(Qt::MatchContains);
    m_cityCombo->setCompleter(m_cityCompleter);
    m_homeCityLabel = new QLabel(tr("Home city:"));
    form->addRow(m_homeCityLabel, m_cityCombo);

    m_detectBtn = new QPushButton(tr("Detect Location"));
    m_detectMenu = new QMenu(this);
    m_detectMenu->addAction(tr("System (GPS/WiFi)"), [this]() {
        if (m_location) m_location->requestOnceSystem();
    });
    m_detectMenu->addAction(tr("IP (approximate)"), [this]() {
        m_ipLocator.request();
    });
    m_detectBtn->setMenu(m_detectMenu);
    form->addRow(QString(), m_detectBtn);

    m_autoStartCheck = new QCheckBox(tr("Auto-detect on startup"));
    m_autoStartCheck->setChecked(m_config->autoDetectOnStart());
    form->addRow(m_autoStartCheck);

    m_coordLabel = new QLabel;
    refreshCoordLabel();
    form->addRow(QString(), m_coordLabel);

    // Wire city selection AFTER populate so the initial addItems don't fire.
    connect(m_cityCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int) {
        const auto c = CityDatabase::instance().findByDisplay(m_cityCombo->currentText());
        if (c) onLocationFound(c->lat, c->lon, c->name);
    });
    connect(m_cityCompleter, QOverload<const QString &>::of(&QCompleter::activated),
            this, [this](const QString &text) {
        const auto c = CityDatabase::instance().findByDisplay(text);
        if (c) onLocationFound(c->lat, c->lon, c->name);
    });

    // Live location detection -> update config + globe marker.
    if (m_location)
        connect(m_location, &LocationProvider::locationChanged, this,
                [this](double lat, double lon) { onLocationFound(lat, lon, QString()); });
    connect(&m_ipLocator, &IpLocator::located, this, &SettingsDialog::onLocationFound);

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

void SettingsDialog::refreshCoordLabel() {
    const double lat = m_config ? m_config->homeLatitude() : 0.0;
    const double lon = m_config ? m_config->homeLongitude() : 0.0;
    // 0,0 is the config default => treat as "not set" (a real 0,0 fix is rare).
    if (lat == 0.0 && lon == 0.0)
        m_coordLabel->setText(tr("not set"));
    else
        m_coordLabel->setText(QStringLiteral("%1\xc2\xb0, %2\xc2\xb0")
                                  .arg(lat, 0, 'f', 4).arg(lon, 0, 'f', 4));
}

void SettingsDialog::onLocationFound(double lat, double lon, const QString &city) {
    if (!m_config) return;
    m_config->setHomeLatitude(lat);
    m_config->setHomeLongitude(lon);
    m_config->setShowHomeMarker(true);   // a successful detect/show enables the marker
    m_config->save();
    refreshCoordLabel();
    // Best-effort: reflect a matching city in the combo. The combo lists
    // "City, Country" but the detect signals only carry a bare city name, so
    // match by prefix (System detect passes an empty city => no change).
    if (m_cityCombo && !city.isEmpty()) {
        const int idx = m_cityCombo->findText(city, Qt::MatchStartsWith);
        if (idx >= 0) m_cityCombo->setCurrentIndex(idx);
    }
    emit settingsChanged();   // live-update the globe/map marker
}

void SettingsDialog::retranslateUi() {
    setWindowTitle(tr("Settings"));
    m_gridCheck->setText(tr("Show Grid"));
    m_alwaysOnTopCheck->setText(tr("Always on Top"));
    m_markerCheck->setText(tr("Show home marker"));
    m_autoStartCheck->setText(tr("Auto-detect on startup"));
    m_langLabel->setText(tr("Language:"));
    m_viewModeLabel->setText(tr("View Mode:"));
    m_homeCityLabel->setText(tr("Home city:"));
    m_detectBtn->setText(tr("Detect Location"));
    const auto acts = m_detectMenu->actions();
    if (acts.size() > 0) acts.at(0)->setText(tr("System (GPS/WiFi)"));
    if (acts.size() > 1) acts.at(1)->setText(tr("IP (approximate)"));
    m_spinLabel->setText(tr("Spin Speed:"));
    m_simpleNightRadio->setText(tr("Simple Night"));
    m_textureNightRadio->setText(tr("Realistic Night (city lights)"));
    m_nightGroup->setTitle(tr("Night Mode"));
    // Combo item texts (re-applied in place; data values are untouched).
    m_viewModeCombo->setItemText(0, tr("Globe (3D)"));
    m_viewModeCombo->setItemText(1, tr("Flat Map"));
    m_languageCombo->setItemText(0, tr("English"));
    refreshCoordLabel();   // retranslates the "not set" fallback
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
    m_config->setAutoDetectOnStart(m_autoStartCheck->isChecked());
    m_config->save();
    emit settingsChanged();
    QDialog::accept();
}
