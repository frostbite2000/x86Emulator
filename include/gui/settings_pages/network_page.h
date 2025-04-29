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

#ifndef NETWORK_SETTINGS_PAGE_H
#define NETWORK_SETTINGS_PAGE_H

#include "gui/settings_dialog.h"

/**
 * @brief The Network settings page for configuring network adapters
 */
class NetworkSettingsPage : public SettingsPage {
    Q_OBJECT
public:
    explicit NetworkSettingsPage(Emulator* emulator, QWidget* parent = nullptr);
    ~NetworkSettingsPage() override;

    void apply() override;
    void load() override;
    void reset() override;
    bool hasChanges() const override;

private slots:
    void onNetworkTypeChanged(int index);
    void onNetworkCardChanged(int index);
    void onConfigureCardClicked();

private:
    void setupUi();
    void populateNetworkTypes();
    void populateNetworkCards();
    void updateNetworkDependentControls();
    
    Emulator* m_emulator;
    
    // Original values for change detection
    QString m_origNetworkType;
    QString m_origNetworkCard;
    QString m_origMacAddress;
    
    // UI Controls
    QComboBox* m_networkTypeCombo;
    QComboBox* m_networkCardCombo;
    QPushButton* m_configureCardButton;
    QLineEdit* m_macAddressEdit;
    QCheckBox* m_pcap1Check;
    QCheckBox* m_pcap2Check;
};

#endif // NETWORK_SETTINGS_PAGE_H