#pragma once
#include <QDialog>

class ConfigManager;
class QCheckBox;
class QRadioButton;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QSlider;
class QLabel;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(ConfigManager *config, QWidget *parent = nullptr);

    // Re-apply all translated strings (call after swapping the QTranslator).
    void retranslateUi();

signals:
    void settingsChanged();

private slots:
    void accept() override;
    void onRotationSlider(int value);

private:
    ConfigManager *m_config = nullptr;
    QCheckBox *m_gridCheck = nullptr;
    QCheckBox *m_alwaysOnTopCheck = nullptr;
    QCheckBox *m_markerCheck = nullptr;
    QCheckBox *m_autoLocCheck = nullptr;
    QRadioButton *m_simpleNightRadio = nullptr;
    QRadioButton *m_textureNightRadio = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QComboBox *m_viewModeCombo = nullptr;
    QDoubleSpinBox *m_latSpin = nullptr;
    QDoubleSpinBox *m_lonSpin = nullptr;
    QSlider *m_rotationSlider = nullptr;
    QLabel *m_rotationValueLabel = nullptr;
    QLabel *m_langLabel = nullptr;
    QLabel *m_viewModeLabel = nullptr;
    QLabel *m_homeLocLabel = nullptr;
    QLabel *m_spinLabel = nullptr;
    QGroupBox *m_nightGroup = nullptr;

    void setupUi();
};
