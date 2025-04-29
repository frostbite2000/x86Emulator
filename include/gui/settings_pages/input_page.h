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

#ifndef INPUT_SETTINGS_PAGE_H
#define INPUT_SETTINGS_PAGE_H

#include "gui/settings_dialog.h"

/**
 * @brief The Input settings page for configuring keyboards, mice, and joysticks
 */
class InputSettingsPage : public SettingsPage {
    Q_OBJECT
public:
    explicit InputSettingsPage(Emulator* emulator, QWidget* parent = nullptr);
    ~InputSettingsPage() override;

    void apply() override;
    void load() override;
    void reset() override;
    bool hasChanges() const override;

private slots:
    void onMouseTypeChanged(int index);
    void onJoystickConfigureClicked();

private:
    void setupUi();
    void populateKeyboardTypes();
    void populateMouseTypes();
    void populateJoysticks();
    
    Emulator* m_emulator;
    
    // Original values for change detection
    QString m_origKeyboardType;
    QString m_origMouseType;
    QString m_origJoystick1;
    QString m_origJoystick2;
    QString m_origJoystick3;
    QString m_origJoystick4;
    
    // UI Controls
    QTabWidget* m_tabWidget;
    
    // Keyboard Tab
    QWidget* m_keyboardTab;
    QComboBox* m_keyboardTypeCombo;
    
    // Mouse Tab
    QWidget* m_mouseTab;
    QComboBox* m_mouseTypeCombo;
    
    // Joystick Tab
    QWidget* m_joystickTab;
    QComboBox* m_joystick1Combo;
    QComboBox* m_joystick2Combo;
    QComboBox* m_joystick3Combo;
    QComboBox* m_joystick4Combo;
    QPushButton* m_joystickConfigureButton;
    
    // Mapping Tab
    QWidget* m_mappingTab;
    QPushButton* m_keyboardMappingButton;
};

#endif // INPUT_SETTINGS_PAGE_H