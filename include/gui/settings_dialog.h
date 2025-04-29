/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2024 frostbite2000
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QSettings>
#include <QIcon>
#include <QList>
#include <QHash>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QTableWidget>

#include "emulator.h"
#include "config_manager.h"

// Forward declarations
class MachineSettingsPage;
class DisplaySettingsPage;
class InputSettingsPage;
class SoundSettingsPage;
class NetworkSettingsPage;
class PortsSettingsPage;
class StorageControllerSettingsPage;
class HardDiskSettingsPage;
class FloppyCdromSettingsPage;
class OtherRemovableSettingsPage;
class OtherPeripheralsSettingsPage;

/**
 * @brief A settings page interface that all settings pages must implement.
 */
class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent = nullptr) : QWidget(parent) {}
    virtual ~SettingsPage() {}

    /**
     * @brief Apply the settings changes.
     */
    virtual void apply() = 0;

    /**
     * @brief Load current settings.
     */
    virtual void load() = 0;

    /**
     * @brief Reset the page to default settings.
     */
    virtual void reset() = 0;

    /**
     * @brief Check if the settings have been changed.
     * @return True if settings have changed, false otherwise.
     */
    virtual bool hasChanges() const = 0;

protected:
    /**
     * @brief Create and return a standard form layout.
     * @return A pointer to a QFormLayout instance with proper styling.
     */
    QFormLayout* createFormLayout() {
        QFormLayout* layout = new QFormLayout;
        layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->setSpacing(10);
        return layout;
    }
};

/**
 * @brief The main settings dialog that contains all settings pages.
 */
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(Emulator* emulator, QWidget* parent = nullptr);
    ~SettingsDialog();

private slots:
    /**
     * @brief Handle the OK button click.
     */
    void accept() override;

    /**
     * @brief Handle the Cancel button click.
     */
    void reject() override;

    /**
     * @brief Handle when a different category is selected.
     * @param currentItem The currently selected item
     */
    void onCategoryChanged(QListWidgetItem* current);

private:
    /**
     * @brief Initialize the UI components.
     */
    void setupUi();

    /**
     * @brief Load settings from the configuration file.
     */
    void loadSettings();

    /**
     * @brief Apply the settings changes to the emulator.
     */
    void applySettings();

    /**
     * @brief Add a settings page to the dialog.
     * @param page The settings page to add
     * @param title The title of the page
     * @param iconName The name of the icon for the page
     */
    void addPage(SettingsPage* page, const QString& title, const QString& iconName);

    Emulator* m_emulator;
    ConfigManager* m_configManager;

    QListWidget* m_categoriesList;
    QStackedWidget* m_pagesStack;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Settings pages
    MachineSettingsPage* m_machinePage;
    DisplaySettingsPage* m_displayPage;
    InputSettingsPage* m_inputPage;
    SoundSettingsPage* m_soundPage;
    NetworkSettingsPage* m_networkPage;
    PortsSettingsPage* m_portsPage;
    StorageControllerSettingsPage* m_storageCtrlPage;
    HardDiskSettingsPage* m_hardDiskPage;
    FloppyCdromSettingsPage* m_floppyCdromPage;
    OtherRemovableSettingsPage* m_otherRemovablePage;
    OtherPeripheralsSettingsPage* m_otherPeripheralsPage;

    QList<SettingsPage*> m_pages;
};

#endif // SETTINGS_DIALOG_H