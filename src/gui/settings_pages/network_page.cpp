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

#include "gui/settings_pages/network_page.h"
#include "config_manager.h"
#include "emulator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

NetworkSettingsPage::NetworkSettingsPage(Emulator* emulator, QWidget* parent)
    : SettingsPage(parent),
      m_emulator(emulator)
{
    setupUi();
    
    populateNetworkTypes();
    populateNetworkCards();
    updateNetworkDependentControls();
}

NetworkSettingsPage::~NetworkSettingsPage()
{
}

void NetworkSettingsPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create form layout
    QFormLayout* formLayout = createFormLayout();
    
    // Network type combobox
    m_networkTypeCombo = new QComboBox(this);
    connect(m_networkTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NetworkSettingsPage::onNetworkTypeChanged);
    formLayout->addRow(tr("Network type:"), m_networkTypeCombo);
    
    // Network card combobox with Configure button
    QHBoxLayout* networkCardLayout = new QHBoxLayout();
    m_networkCardCombo = new QComboBox(this);
    connect(m_networkCardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NetworkSettingsPage::onNetworkCardChanged);
    networkCardLayout->addWidget(m_networkCardCombo);
    
    m_configureCardButton = new QPushButton(tr("Configure"), this);
    m_configureCardButton->setFixedWidth(100);
    connect(m_configureCardButton, &QPushButton::clicked, this, &NetworkSettingsPage::onConfigureCardClicked);
    networkCardLayout->addWidget(m_configureCardButton);
    
    formLayout->addRow(tr("Network card:"), networkCardLayout);
    
    // MAC Address edit
    m_macAddressEdit = new QLineEdit(this);
    m_macAddressEdit->setInputMask("HH:HH:HH:HH:HH:HH;_");
    formLayout->addRow(tr("MAC Address:"), m_macAddressEdit);
    
    // PCap options
    QGroupBox* pcapGroup = new QGroupBox(tr("PCap Options"), this);
    QVBoxLayout* pcapLayout = new QVBoxLayout(pcapGroup);
    
    m_pcap1Check = new QCheckBox(tr("Use direct hardware access (Administrator rights required)"), pcapGroup);
    pcapLayout->addWidget(m_pcap1Check);
    
    m_pcap2Check = new QCheckBox(tr("Promiscuous mode (see all network traffic)"), pcapGroup);
    pcapLayout->addWidget(m_pcap2Check);
    
    formLayout->addRow("", pcapGroup);
    
    // Add a note about network setup
    QLabel* networkNote = new QLabel(
        tr("Note: To connect to the internet, set the network type to PCap or SLiRP, "
           "and select an appropriate network card for the machine you are emulating. "
           "PCap requires administrator privileges but provides better compatibility "
           "with legacy software that uses direct hardware access."),
        this);
    networkNote->setWordWrap(true);
    formLayout->addRow("", networkNote);
    
    // Add form layout to main layout
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
}

void NetworkSettingsPage::populateNetworkTypes()
{
    m_networkTypeCombo->clear();
    m_networkTypeCombo->addItem(tr("None"));
    m_networkTypeCombo->addItem(tr("PCap (direct)"));
    m_networkTypeCombo->addItem(tr("SLiRP (user-mode)"));
    m_networkTypeCombo->addItem(tr("VDE (Virtual Distributed Ethernet)"));
}

void NetworkSettingsPage::populateNetworkCards()
{
    m_networkCardCombo->clear();
    m_networkCardCombo->addItem(tr("None"));
    m_networkCardCombo->addItem(tr("Western Digital WD8003E"));
    m_networkCardCombo->addItem(tr("Novell NE1000"));
    m_networkCardCombo->addItem(tr("Novell NE2000"));
    m_networkCardCombo->addItem(tr("Realtek RTL8029AS"));
    m_networkCardCombo->addItem(tr("3Com 3C503"));
    m_networkCardCombo->addItem(tr("3Com 3C509B"));
    m_networkCardCombo->addItem(tr("3Com 3C90x"));
    m_networkCardCombo->addItem(tr("Intel PRO/100"));
    m_networkCardCombo->addItem(tr("Realtek RTL8139C+"));
}

void NetworkSettingsPage::updateNetworkDependentControls()
{
    // Enable/disable network card selection based on network type
    bool networkEnabled = m_networkTypeCombo->currentText() != "None";
    m_networkCardCombo->setEnabled(networkEnabled);
    m_configureCardButton->setEnabled(networkEnabled && m_networkCardCombo->currentText() != "None");
    m_macAddressEdit->setEnabled(networkEnabled && m_networkCardCombo->currentText() != "None");
    
    // PCap options only available with PCap network type
    bool pcapEnabled = m_networkTypeCombo->currentText() == "PCap (direct)";
    m_pcap1Check->setEnabled(pcapEnabled);
    m_pcap2Check->setEnabled(pcapEnabled);
}

void NetworkSettingsPage::onNetworkTypeChanged(int index)
{
    Q_UNUSED(index);
    updateNetworkDependentControls();
}

void NetworkSettingsPage::onNetworkCardChanged(int index)
{
    Q_UNUSED(index);
    updateNetworkDependentControls();
    
    // Generate a random MAC address if the field is empty and a card is selected
    if (m_macAddressEdit->text().isEmpty() || m_macAddressEdit->text() == ":::::") {
        if (m_networkCardCombo->currentText() != "None") {
            // Generate a random MAC address with a valid OUI prefix
            // Use locally administered address space (first byte has bit 1 set)
            uint8_t mac[6];
            mac[0] = 0x02; // Locally administered
            
            // Random values for the rest
            for (int i = 1; i < 6; i++) {
                mac[i] = qrand() % 256;
            }
            
            QString macStr = QString("%1:%2:%3:%4:%5:%6")
                .arg(mac[0], 2, 16, QChar('0'))
                .arg(mac[1], 2, 16, QChar('0'))
                .arg(mac[2], 2, 16, QChar('0'))
                .arg(mac[3], 2, 16, QChar('0'))
                .arg(mac[4], 2, 16, QChar('0'))
                .arg(mac[5], 2, 16, QChar('0'));
            
            m_macAddressEdit->setText(macStr.toUpper());
        }
    }
}

void NetworkSettingsPage::onConfigureCardClicked()
{
    QMessageBox::information(this, tr("Network Card Configuration"), 
                           tr("This would open a network card-specific configuration dialog."));
}

void NetworkSettingsPage::load()
{
    ConfigManager* config = m_emulator->getConfigManager();
    
    // Load network type
    m_origNetworkType = config->getString("network", "type", "None");
    int networkTypeIndex = m_networkTypeCombo->findText(m_origNetworkType);
    if (networkTypeIndex >= 0) {
        m_networkTypeCombo->setCurrentIndex(networkTypeIndex);
    }
    
    // Load network card
    m_origNetworkCard = config->getString("network", "card", "None");
    int networkCardIndex = m_networkCardCombo->findText(m_origNetworkCard);
    if (networkCardIndex >= 0) {
        m_networkCardCombo->setCurrentIndex(networkCardIndex);
    }
    
    // Load MAC address
    m_origMacAddress = config->getString("network", "mac_address", "");
    m_macAddressEdit->setText(m_origMacAddress);
    
    // Load PCap options
    bool pcap1 = config->getBool("network", "pcap_direct_access", false);
    m_pcap1Check->setChecked(pcap1);
    
    bool pcap2 = config->getBool("network", "pcap_promiscuous", true);
    m_pcap2Check->setChecked(pcap2);
    
    // Update dependent controls
    updateNetworkDependentControls();
}

void NetworkSettingsPage::apply()
{
    ConfigManager* config = m_emulator->getConfigManager();
    
    // Save network type
    config->setString("network", "type", m_networkTypeCombo->currentText());
    
    // Save network card
    config->setString("network", "card", m_networkCardCombo->currentText());
    
    // Save MAC address
    QString macAddress = m_macAddressEdit->text();
    if (!macAddress.isEmpty() && macAddress != ":::::") {
        config->setString("network", "mac_address", macAddress);
    }
    
    // Save PCap options
    config->setBool("network", "pcap_direct_access", m_pcap1Check->isChecked());
    config->setBool("network", "pcap_promiscuous", m_pcap2Check->isChecked());
}

void NetworkSettingsPage::reset()
{
    // Reset to defaults
    m_networkTypeCombo->setCurrentIndex(0); // None
    m_networkCardCombo->setCurrentIndex(0); // None
    m_macAddressEdit->clear();
    m_pcap1Check->setChecked(false);
    m_pcap2Check->setChecked(true);
    
    // Update dependent controls
    updateNetworkDependentControls();
}

bool NetworkSettingsPage::hasChanges() const
{
    // Check if any settings have changed
    if (m_networkTypeCombo->currentText() != m_origNetworkType) return true;
    if (m_networkCardCombo->currentText() != m_origNetworkCard) return true;
    
    QString currentMac = m_macAddressEdit->text();
    if (currentMac != m_origMacAddress) return true;
    
    bool pcap1 = m_pcap1Check->isChecked();
    bool pcap2 = m_pcap2Check->isChecked();
    
    ConfigManager* config = m_emulator->getConfigManager();
    bool origPcap1 = config->getBool("network", "pcap_direct_access", false);
    bool origPcap2 = config->getBool("network", "pcap_promiscuous", true);
    
    if (pcap1 != origPcap1) return true;
    if (pcap2 != origPcap2) return true;
    
    return false;
}