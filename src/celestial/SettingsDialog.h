#pragma once
#include <QDialog>

class ConfigManager;
class QCheckBox;
class QRadioButton;
class QComboBox;
class QSlider;
class QLabel;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(ConfigManager *config, QWidget *parent = nullptr);

signals:
    void settingsChanged();

private slots:
    void accept() override;
    void onRotationSlider(int value);

private:
    ConfigManager *m_config = nullptr;
    QCheckBox *m_gridCheck = nullptr;
    QCheckBox *m_alwaysOnTopCheck = nullptr;
    QRadioButton *m_simpleNightRadio = nullptr;
    QRadioButton *m_textureNightRadio = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QSlider *m_rotationSlider = nullptr;
    QLabel *m_rotationValueLabel = nullptr;
    QComboBox *m_viewModeCombo = nullptr;
    QCheckBox *m_locationCheck = nullptr;   // location opt-in (home marker)

    void setupUi();
};
