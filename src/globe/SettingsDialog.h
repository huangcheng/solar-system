#pragma once
#include <QDialog>

class ConfigManager;
class QCheckBox;
class QRadioButton;
class QComboBox;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(ConfigManager *config, QWidget *parent = nullptr);

signals:
    void settingsChanged();

private slots:
    void accept() override;

private:
    ConfigManager *m_config = nullptr;
    QCheckBox *m_gridCheck = nullptr;
    QRadioButton *m_simpleNightRadio = nullptr;
    QRadioButton *m_textureNightRadio = nullptr;
    QComboBox *m_languageCombo = nullptr;

    void setupUi();
};
