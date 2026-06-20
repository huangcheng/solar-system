#pragma once
#include <QDialog>
#include "IpLocator.h"

class ConfigManager;
class LocationProvider;
class QCheckBox;
class QRadioButton;
class QComboBox;
class QCompleter;
class QPushButton;
class QMenu;
class QGroupBox;
class QSlider;
class QLabel;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    // `location` defaults to nullptr so the dialog can be constructed without a
    // live system provider (the System detect action then no-ops); main.cpp
    // passes the real LocationProvider so System detection works.
    explicit SettingsDialog(ConfigManager *config, QWidget *parent = nullptr,
                            LocationProvider *location = nullptr);

    // Re-apply all translated strings (call after swapping the QTranslator).
    void retranslateUi();

signals:
    void settingsChanged();

private slots:
    void accept() override;
    void onRotationSlider(int value);
    void onLocationFound(double lat, double lon, const QString &city);

private:
    ConfigManager *m_config = nullptr;
    LocationProvider *m_location = nullptr;
    IpLocator m_ipLocator;

    QCheckBox *m_gridCheck = nullptr;
    QCheckBox *m_alwaysOnTopCheck = nullptr;
    QCheckBox *m_markerCheck = nullptr;
    QCheckBox *m_autoStartCheck = nullptr;
    QRadioButton *m_simpleNightRadio = nullptr;
    QRadioButton *m_textureNightRadio = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QComboBox *m_viewModeCombo = nullptr;
    QComboBox *m_cityCombo = nullptr;
    QCompleter *m_cityCompleter = nullptr;
    QPushButton *m_detectBtn = nullptr;
    QMenu *m_detectMenu = nullptr;
    QSlider *m_rotationSlider = nullptr;
    QLabel *m_rotationValueLabel = nullptr;
    QLabel *m_langLabel = nullptr;
    QLabel *m_viewModeLabel = nullptr;
    QLabel *m_homeCityLabel = nullptr;
    QLabel *m_coordLabel = nullptr;
    QLabel *m_spinLabel = nullptr;
    QGroupBox *m_nightGroup = nullptr;

    void setupUi();
    void refreshCoordLabel();
};
