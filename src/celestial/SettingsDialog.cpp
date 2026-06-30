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
#include <QDoubleSpinBox>
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
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

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

    m_launchOnLoginCheck = new QCheckBox(tr("Start with system"));
    m_launchOnLoginCheck->setChecked(m_config->launchOnLogin());
    form->addRow(m_launchOnLoginCheck);

    m_markerCheck = new QCheckBox(tr("Show home marker"));
    m_markerCheck->setChecked(m_config->showHomeMarker());
    form->addRow(m_markerCheck);

    // --- Home location group (city selector + fetch + auto-start + lat/lon) ---

    m_homeGroup = new QGroupBox(tr("Home Location"));
    auto *homeLayout = new QFormLayout(m_homeGroup);
    homeLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    homeLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

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
    homeLayout->addRow(m_homeCityLabel, m_cityCombo);

    // Detect method selector (combo, like Home city) + Fetch button.
    m_detectCombo = new QComboBox;
    m_detectCombo->addItem(tr("System (GPS/WiFi)"), QStringLiteral("system"));
    m_detectCombo->addItem(tr("IP (approximate)"), QStringLiteral("ip"));
    m_detectCombo->setCurrentIndex(0);
    m_fetchBtn = new QPushButton(tr("Fetch"));
    auto *detectRow = new QWidget;
    auto *detectLay = new QHBoxLayout(detectRow);
    detectLay->setContentsMargins(0, 0, 0, 0);
    detectLay->addWidget(m_detectCombo, 1);
    detectLay->addWidget(m_fetchBtn);
    homeLayout->addRow(QString(), detectRow);

    connect(m_fetchBtn, &QPushButton::clicked, this, [this]() {
        m_statusLabel->setText(tr("Fetching location..."));
        const QString method = m_detectCombo->currentData().toString();
        if (method == QStringLiteral("system")) {
            m_systemFetchPending = true;
            if (m_location) m_location->requestOnceSystem();
        } else {
            m_ipLocator.request();
        }
    });

    m_latSpin = new QDoubleSpinBox;
    m_latSpin->setRange(-90.0, 90.0);
    m_latSpin->setDecimals(4);
    m_latSpin->setSuffix(tr("\xc2\xb0"));   // degree sign
    m_latSpin->setValue(m_config->homeLatitude());
    m_latLabel = new QLabel(tr("Latitude:"));

    m_lonSpin = new QDoubleSpinBox;
    m_lonSpin->setRange(-180.0, 180.0);
    m_lonSpin->setDecimals(4);
    m_lonSpin->setSuffix(tr("\xc2\xb0"));
    m_lonSpin->setValue(m_config->homeLongitude());
    m_lonLabel = new QLabel(tr("Longitude:"));

    // Keep the coordinate labels readable on platforms with larger system fonts.
    m_latLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_lonLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_latSpin->setMinimumWidth(90);
    m_lonSpin->setMinimumWidth(90);

    auto *coordsRow = new QWidget;
    coordsRow->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    auto *coordsLay = new QHBoxLayout(coordsRow);
    coordsLay->setContentsMargins(0, 0, 0, 0);
    coordsLay->addWidget(m_latLabel);
    coordsLay->addWidget(m_latSpin, 1);
    coordsLay->addSpacing(8);
    coordsLay->addWidget(m_lonLabel);
    coordsLay->addWidget(m_lonSpin, 1);
    homeLayout->addRow(QString(), coordsRow);

    m_autoStartCheck = new QCheckBox(tr("Auto-detect on startup"));
    m_autoStartCheck->setChecked(m_config->autoDetectOnStart());
    homeLayout->addRow(m_autoStartCheck);

    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_statusLabel->setMinimumWidth(240);
    homeLayout->addRow(QString(), m_statusLabel);

    // Wire city selection AFTER populate so the initial addItems don't fire.
    connect(m_cityCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int) {
        const auto c = CityDatabase::instance().findByDisplay(m_cityCombo->currentText());
        if (c) commitLocation(c->lat, c->lon);   // combo already shows the picked city
    });
    connect(m_cityCompleter, QOverload<const QString &>::of(&QCompleter::activated),
            this, [this](const QString &text) {
        const auto c = CityDatabase::instance().findByDisplay(text);
        if (c) commitLocation(c->lat, c->lon);
    });
    // Manual lat/lon entry -> update config/marker + best-effort exact city match.
    connect(m_latSpin, &QDoubleSpinBox::editingFinished, this, &SettingsDialog::onCoordsEdited);
    connect(m_lonSpin, &QDoubleSpinBox::editingFinished, this, &SettingsDialog::onCoordsEdited);

    // Live location detection -> update spinboxes/config + best-effort combo.
    if (m_location) {
        connect(m_location, &LocationProvider::locationChanged, this,
                [this](double lat, double lon) {
                    m_systemFetchPending = false;
                    onLocationFound(lat, lon, QString());
                });
        // If a user-initiated System fetch cannot get a fix (permission denied,
        // no backend, or timeout), surface it and fall back to IP automatically
        // so the user still ends up with a location.
        connect(m_location, &LocationProvider::locationFailed, this, [this]() {
            if (!m_systemFetchPending)
                return;   // not a user-initiated fetch (e.g. startup) -> ignore here
            m_systemFetchPending = false;
            m_statusLabel->setText(tr("System location unavailable; trying IP..."));
            m_ipLocator.request();
        });
    }
    connect(&m_ipLocator, &IpLocator::located, this, &SettingsDialog::onLocationFound);
    connect(&m_ipLocator, &IpLocator::failed, this, [this]() {
        m_statusLabel->setText(tr("IP location lookup failed."));
    });

    form->addRow(m_homeGroup);

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

void SettingsDialog::commitLocation(double lat, double lon) {
    if (!m_config) return;
    // setValue() is programmatic -> does NOT emit editingFinished, so no feedback loop.
    if (m_latSpin) m_latSpin->setValue(lat);
    if (m_lonSpin) m_lonSpin->setValue(lon);
    m_config->setHomeLatitude(lat);
    m_config->setHomeLongitude(lon);
    m_config->setShowHomeMarker(true);   // a successful detect/select enables the marker
    m_config->save();
    emit settingsChanged();   // live-update the globe/map marker
}

void SettingsDialog::onLocationFound(double lat, double lon, const QString &city) {
    commitLocation(lat, lon);
    // Update the city combo to the nearest bundled city (handles system fixes
    // that carry no city name, and refines IP results to a known city).
    if (m_cityCombo) {
        const auto nearest = CityDatabase::instance().findNearest(lat, lon);
        if (nearest) {
            const int idx = m_cityCombo->findText(nearest->display());
            if (idx >= 0) m_cityCombo->setCurrentIndex(idx);
        } else if (!city.isEmpty()) {
            // Fallback to the old prefix match if the DB is empty.
            const int idx = m_cityCombo->findText(city, Qt::MatchStartsWith);
            if (idx >= 0) m_cityCombo->setCurrentIndex(idx);
        }
    }
    if (m_statusLabel)
        m_statusLabel->setText(tr("Location updated: %1, %2")
                                     .arg(lat, 0, 'f', 4).arg(lon, 0, 'f', 4));
}

void SettingsDialog::onCoordsEdited() {
    const double lat = m_latSpin->value();
    const double lon = m_lonSpin->value();
    commitLocation(lat, lon);
    // Snap the city combo to the nearest bundled city for the typed coordinates.
    // Hand-typed lat/lon almost never match a city's stored coords exactly, so
    // the old exact-match test left the combo stale while the marker moved.
    if (m_cityCombo) {
        const auto nearest = CityDatabase::instance().findNearest(lat, lon);
        if (nearest) {
            const int idx = m_cityCombo->findText(nearest->display());
            if (idx >= 0) m_cityCombo->setCurrentIndex(idx);   // programmatic: no activated re-fire
        }
    }
}

void SettingsDialog::retranslateUi() {
    setWindowTitle(tr("Settings"));
    m_gridCheck->setText(tr("Show Grid"));
    m_alwaysOnTopCheck->setText(tr("Always on Top"));
    m_launchOnLoginCheck->setText(tr("Start with system"));
    m_markerCheck->setText(tr("Show home marker"));
    m_autoStartCheck->setText(tr("Auto-detect on startup"));
    m_langLabel->setText(tr("Language:"));
    m_viewModeLabel->setText(tr("View Mode:"));
    m_homeCityLabel->setText(tr("Home city:"));
    m_fetchBtn->setText(tr("Fetch"));
    m_detectCombo->setItemText(0, tr("System (GPS/WiFi)"));
    m_detectCombo->setItemText(1, tr("IP (approximate)"));
    m_latLabel->setText(tr("Latitude:"));
    m_lonLabel->setText(tr("Longitude:"));
    m_latSpin->setSuffix(tr("\xc2\xb0"));
    m_lonSpin->setSuffix(tr("\xc2\xb0"));
    m_lonLabel->setText(tr("Longitude:"));
    m_latSpin->setSuffix(tr("\xc2\xb0"));
    m_lonSpin->setSuffix(tr("\xc2\xb0"));
    m_statusLabel->clear();
    m_spinLabel->setText(tr("Spin Speed:"));
    m_simpleNightRadio->setText(tr("Simple Night"));
    m_textureNightRadio->setText(tr("Realistic Night (city lights)"));
    m_nightGroup->setTitle(tr("Night Mode"));
    m_homeGroup->setTitle(tr("Home Location"));
    // Combo item texts (re-applied in place; data values are untouched).
    m_viewModeCombo->setItemText(0, tr("Globe (3D)"));
    m_viewModeCombo->setItemText(1, tr("Flat Map"));
    m_languageCombo->setItemText(0, tr("English"));
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
    const bool launch = m_launchOnLoginCheck->isChecked();
    m_config->setLaunchOnLogin(launch);
    ConfigManager::applyLaunchOnLogin(launch);
    // Capture the latest lat/lon from the spinboxes (they are normally already
    // in config via editingFinished, but accept is the authoritative write).
    m_config->setHomeLatitude(m_latSpin->value());
    m_config->setHomeLongitude(m_lonSpin->value());
    m_config->save();
    emit settingsChanged();
    QDialog::accept();
}
