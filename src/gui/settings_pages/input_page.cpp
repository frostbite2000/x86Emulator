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

#include "gui/settings_pages/input_page.h"
#include "config_manager.h"
#include "emulator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

InputSettingsPage::InputSettingsPage(Emulator* emulator, QWidget* parent)
    : SettingsPage(parent),
      m_emulator(emulator)
{
    setupUi();
    
    populateKeyboardTypes();
    populateMouseTypes();
    populateJoysticks();
}

InputSettingsPage::~InputSettingsPage()
{
}

void InputSettingsPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    // Keyboard tab
    m_keyboardTab = new QWidget(m_tabWidget);
    QFormLayout* keyboardLayout = createFormLayout();
    
    // Keyboard type combobox
    m_keyboardTypeCombo = new QComboBox(m_keyboardTab);
    keyboardLayout->addRow(tr("Keyboard type:"), m_keyboardTypeCombo);
    
    // Add a description label
    QLabel* keyboardDesc = new QLabel(tr("Select the type of keyboard to emulate."), m_keyboardTab);
    keyboardDesc->setWordWrap(true);
    keyboardLayout->addRow("", keyboardDesc);
    
    m_keyboardTab->setLayout(keyboardLayout);
    m_tabWidget->addTab(m_keyboardTab, tr("Keyboard"));
    
    // Mouse tab
    m_mouseTab = new QWidget(m_tabWidget);
    QFormLayout* mouseLayout = createFormLayout();
    
    // Mouse type combobox
    m_mouseTypeCombo = new QComboBox(m_mouseTab);
    connect(m_mouseTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InputSettingsPage::onMouseTypeChanged);
    mouseLayout->addRow(tr("Mouse type:"), m_mouseTypeCombo);
    
    // Add a description label
    QLabel* mouseDesc = new QLabel(tr("Select the type of mouse to emulate. If using a physical serial mouse, select the appropriate COM port in the Ports configuration."), m_mouseTab);
    mouseDesc->setWordWrap(true);
    mouseLayout->addRow("", mouseDesc);
    
    m_mouseTab->setLayout(mouseLayout);
    m_tabWidget->addTab(m_mouseTab, tr("Mouse"));
    
    // Joystick tab
    m_joystickTab = new QWidget(m_tabWidget);
    QFormLayout* joystickLayout = createFormLayout();
    
    // Joystick comboboxes
    m_joystick1Combo = new QComboBox(m_joystickTab);
    joystickLayout->addRow(tr("Joystick 1:"), m_joystick1Combo);
    
    m_joystick2Combo = new QComboBox(m_joystickTab);
    joystickLayout->addRow(tr("Joystick 2:"), m_joystick2Combo);
    
    m_joystick3Combo = new QComboBox(m_joystickTab);
    joystickLayout->addRow(tr("Joystick 3:"), m_joystick3Combo);
    
    m_joystick4Combo = new QComboBox(m_joystickTab);
    joystickLayout->addRow(tr("Joystick 4:"), m_joystick4Combo);
    
    // Configure joystick button
    m_joystickConfigureButton = new QPushButton(tr("Configure Joysticks"), m_joystickTab);
    connect(m_joystickConfigureButton, &QPushButton::clicked, this, &InputSettingsPage::onJoystickConfigureClicked);
    joystickLayout->addRow("", m_joystickConfigureButton);
    
    m_joystickTab->setLayout(joystickLayout);
    m_tabWidget->addTab(m_joystickTab, tr("Joystick"));
    
    // Mapping tab
    m_mappingTab = new QWidget(m_tabWidget);
    QVBoxLayout* mappingLayout = new QVBoxLayout(m_mappingTab);
    
    // Keyboard mapping button
    m_keyboardMappingButton = new QPushButton(tr("Configure Key Mappings"), m_mappingTab);
    mappingLayout->addWidget(m_keyboardMappingButton, 0, Qt::AlignCenter);
    mappingLayout->addStretch();
    
    m_mappingTab->setLayout(mappingLayout);
    m_tabWidget->addTab(m_mappingTab, tr("Key Mapping"));
    
    // Add tab widget to main layout
    mainLayout->addWidget(m_tabWidget);
}

void InputSettingsPage::populateKeyboardTypes()
{
    m_keyboardTypeCombo->clear();
    m_keyboardTypeCombo->addItem(tr("PC/XT (83-key)"));
    m_keyboardTypeCombo->addItem(tr("PC/AT (84-key)"));
    m_keyboardTypeCombo->addItem(tr("Enhanced (101/102-key)"));
    m_keyboardTypeCombo->addItem(tr("Microsoft Natural Keyboard"));
}

void InputSettingsPage::populateMouseTypes()
{
    m_mouseTypeCombo->clear();
    m_mouseTypeCombo->addItem(tr("None"));
    m_mouseTypeCombo->addItem(tr("2-button serial mouse"));
    m_mouseTypeCombo->addItem(tr("3-button serial mouse"));
    m_mouseTypeCombo->addItem(tr("Microsoft serial mouse"));
    m_mouseTypeCombo->addItem(tr("Logitech serial mouse"));
    m_mouseTypeCombo->addItem(tr("Microsoft bus mouse"));
    m_mouseTypeCombo->addItem(tr("Mouse Systems mouse"));
    m_mouseTypeCombo->addItem(tr("InPort mouse"));
    m_mouseTypeCombo->addItem(tr("PS/2 mouse"));
    m_mouseTypeCombo->addItem(tr("Genius mouse"));
}

void InputSettingsPage::populateJoysticks()
{
    QStringList joystickTypes = {
        tr("None"),
        tr("Standard 2-button joystick"),
        tr("Standard 4-button joystick"),
        tr("Gravis GamePad"),
        tr("CH Flightstick Pro"),
        tr("Microsoft SideWinder"),
        tr("Thrustmaster FCS"),
        tr("Logitech Extreme 3D Pro")
    };
    
    m_joystick1Combo->clear();
    m_joystick2Combo->clear();
    m_joystick3Combo->clear();
    m_joystick4Combo->clear();
    
    for (const QString& type : joystickTypes) {
        m_joystick1Combo->addItem(type);
        m_joystick2Combo->addItem(type);
        m_joystick3Combo->addItem(type);
        m_joystick4Combo->addItem(type);
    }
}

void InputSettingsPage::onMouseTypeChanged(int index)
{
    Q_UNUSED(index);
    // Update any mouse-dependent UI elements if needed
}

void InputSettingsPage::onJoystickConfigureClicked()
{
    QMessageBox::information(this, tr("Joystick Configuration"), 
                           tr("This would open a joystick configuration dialog."));
}

void InputSettingsPage::load()
{
    ConfigManager* config = m_emulator->getConfigManager();
    
    // Load keyboard settings
    m_origKeyboardType = config->getString("keyboard", "type", "Enhanced (101/102-key)");
    int keyboardIndex = m_keyboardTypeCombo->findText(m_origKeyboardType);
    if (keyboardIndex >= 0) {
        m_keyboardTypeCombo->setCurrentIndex(keyboardIndex);
    }
    
    // Load mouse settings
    m_origMouseType = config->getString("mouse", "type", "PS/2 mouse");
    int mouseIndex = m_mouseTypeCombo->findText(m_origMouseType);
    if (mouseIndex >= 0) {
        m_mouseTypeCombo->setCurrentIndex(mouseIndex);
    }
    
    // Load joystick settings
    m_origJoystick1 = config->getString("joystick", "joystick1", "None");
    int joy1Index = m_joystick1Combo->findText(m_origJoystick1);
    if (joy1Index >= 0) {
        m_joystick1Combo->setCurrentIndex(joy1Index);
    }
    
    m_origJoystick2 = config->getString("joystick", "joystick2", "None");
    int joy2Index = m_joystick2Combo->findText(m_origJoystick2);
    if (joy2Index >= 0) {
        m_joystick2Combo->setCurrentIndex(joy2Index);
    }
    
    m_origJoystick3 = config->getString("joystick", "joystick3", "None");
    int joy3Index = m_joystick3Combo->findText(m_origJoystick3);
    if (joy3Index >= 0) {
        m_joystick3Combo->setCurrentIndex(joy3Index);
    }
    
    m_origJoystick4 = config->getString("joystick", "joystick4", "None");
    int joy4Index = m_joystick4Combo->findText(m_origJoystick4);
    if (joy4Index >= 0) {
        m_joystick4Combo->setCurrentIndex(joy4Index);
    }
}

void InputSettingsPage::apply()
{
    ConfigManager* config = m_emulator->getConfigManager();
    
    // Save keyboard settings
    config->setString("keyboard", "type", m_keyboardTypeCombo->currentText());
    
    // Save mouse settings
    config->setString("mouse", "type", m_mouseTypeCombo->currentText());
    
    // Save joystick settings
    config->setString("joystick", "joystick1", m_joystick1Combo->currentText());
    config->setString("joystick", "joystick2", m_joystick2Combo->currentText());
    config->setString("joystick", "joystick3", m_joystick3Combo->currentText());
    config->setString("joystick", "joystick4", m_joystick4Combo->currentText());
}

void InputSettingsPage::reset()
{
    // Reset to defaults
    int keyboardIndex = m_keyboardTypeCombo->findText("Enhanced (101/102-key)");
    if (keyboardIndex >= 0) {
        m_keyboardTypeCombo->setCurrentIndex(keyboardIndex);
    }
    
    int mouseIndex = m_mouseTypeCombo->findText("PS/2 mouse");
    if (mouseIndex >= 0) {
        m_mouseTypeCombo->setCurrentIndex(mouseIndex);
    }
    
    int noneIndex = m_joystick1Combo->findText("None");
    if (noneIndex >= 0) {
        m_joystick1Combo->setCurrentIndex(noneIndex);
        m_joystick2Combo->setCurrentIndex(noneIndex);
        m_joystick3Combo->setCurrentIndex(noneIndex);
        m_joystick4Combo->setCurrentIndex(noneIndex);
    }
}

bool InputSettingsPage::hasChanges() const
{
    // Check if any settings have changed
    if (m_keyboardTypeCombo->currentText() != m_origKeyboardType) return true;
    if (m_mouseTypeCombo->currentText() != m_origMouseType) return true;
    if (m_joystick1Combo->currentText() != m_origJoystick1) return true;
    if (m_joystick2Combo->currentText() != m_origJoystick2) return true;
    if (m_joystick3Combo->currentText() != m_origJoystick3) return true;
    if (m_joystick4Combo->currentText() != m_origJoystick4) return true;
    
    return false;
}