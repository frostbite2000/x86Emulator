/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2025 frostbite2000
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

#include "gui/machine_dialog.h"
#include "machine/machine_config.h"
#include "config_manager.h"
#include "logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QScrollArea>

namespace x86emu {

MachineDialog::MachineDialog(QWidget *parent, std::shared_ptr<ConfigManager> config)
    : QDialog(parent)
    , m_config(config)
{
    setWindowTitle(tr("Select Machine Type"));
    setMinimumSize(600, 500);
    
    initializeDialog();
    populateMachines();
}

MachineDialog::~MachineDialog() = default;

void MachineDialog::initializeDialog()
{
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Machine selection
    QGroupBox* selectionGroup = new QGroupBox(tr("Machine Selection"), this);
    QVBoxLayout* selectionLayout = new QVBoxLayout(selectionGroup);
    
    m_machineCombo = new QComboBox(selectionGroup);
    selectionLayout->addWidget(m_machineCombo);
    
    mainLayout->addWidget(selectionGroup);
    
    // Machine description
    QGroupBox* descriptionGroup = new QGroupBox(tr("Description"), this);
    QVBoxLayout* descriptionLayout = new QVBoxLayout(descriptionGroup);
    
    m_descriptionText = new QTextEdit(descriptionGroup);
    m_descriptionText->setReadOnly(true);
    m_descriptionText->setMaximumHeight(80);
    descriptionLayout->addWidget(m_descriptionText);
    
    mainLayout->addWidget(descriptionGroup);
    
    // Machine specifications
    QGroupBox* specsGroup = new QGroupBox(tr("Specifications"), this);
    QVBoxLayout* specsLayout = new QVBoxLayout(specsGroup);
    
    m_specsTable = new QTableWidget(0, 2, specsGroup);
    m_specsTable->setHorizontalHeaderLabels({"Property", "Value"});
    m_specsTable->horizontalHeader()->setStretchLastSection(true);
    m_specsTable->verticalHeader()->setVisible(false);
    m_specsTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_specsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    specsLayout->addWidget(m_specsTable);
    
    mainLayout->addWidget(specsGroup);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_okButton = new QPushButton(tr("OK"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals and slots
    connect(m_machineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MachineDialog::onMachineSelectionChanged);
    connect(m_okButton, &QPushButton::clicked, this, &MachineDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &MachineDialog::onCancelClicked);
}

void MachineDialog::populateMachines()
{
    // Disconnect signal to avoid triggering updates during population
    disconnect(m_machineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MachineDialog::onMachineSelectionChanged);
    
    // Clear the combo box
    m_machineCombo->clear();
    
    // Get available machines
    auto& machineManager = MachineManager::getInstance();
    auto machines = machineManager.getAvailableMachines();
    
    // Add machines to combo box
    for (const auto& machine : machines) {
        m_machineCombo->addItem(QString::fromStdString(machine->getName()));
    }
    
    // Reconnect signal
    connect(m_machineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MachineDialog::onMachineSelectionChanged);
    
    // Set current machine if configured
    std::string currentMachine = m_config->getString("general", "machine");
    if (!currentMachine.empty()) {
        int index = m_machineCombo->findText(QString::fromStdString(currentMachine));
        if (index >= 0) {
            m_machineCombo->setCurrentIndex(index);
        }
    } else {
        // Default to first machine
        m_machineCombo->setCurrentIndex(0);
    }
    
    // Update details for initial selection
    onMachineSelectionChanged(m_machineCombo->currentIndex());
}

void MachineDialog::updateMachineDetails(std::shared_ptr<MachineConfig> machine)
{
    if (!machine) {
        return;
    }
    
    // Update description
    m_descriptionText->setText(QString::fromStdString(machine->getDescription()));
    
    // Update specifications
    m_specsTable->setRowCount(0);
    
    const auto& spec = machine->getHardwareSpec();
    const auto& bios = machine->getBiosInfo();
    
    // Add hardware specifications
    auto addRow = [this](const QString& property, const QString& value) {
        int row = m_specsTable->rowCount();
        m_specsTable->insertRow(row);
        m_specsTable->setItem(row, 0, new QTableWidgetItem(property));
        m_specsTable->setItem(row, 1, new QTableWidgetItem(value));
    };
    
    addRow(tr("Chipset"), QString::fromStdString(machine->getChipsetName()));
    addRow(tr("Form Factor"), QString::fromStdString(machine->getFormFactorName()));
    addRow(tr("Maximum RAM"), QString("%1 MB").arg(spec.maxRamMB));
    addRow(tr("Onboard Audio"), spec.hasOnboardAudio ? tr("Yes") : tr("No"));
    addRow(tr("Onboard Video"), spec.hasOnboardVideo ? tr("Yes") : tr("No"));
    addRow(tr("Onboard LAN"), spec.hasOnboardLan ? tr("Yes") : tr("No"));
    addRow(tr("IDE Channels"), QString::number(spec.numIdeChannels));
    addRow(tr("Floppy Controller"), spec.hasFloppyController ? tr("Yes") : tr("No"));
    addRow(tr("Serial Ports"), QString::number(spec.numSerialPorts));
    addRow(tr("Parallel Ports"), QString::number(spec.numParallelPorts));
    addRow(tr("USB Ports"), QString::number(spec.numUsbPorts));
    addRow(tr("PS/2 Ports"), spec.hasPs2Ports ? tr("Yes") : tr("No"));
    
    // Add BIOS information
    addRow(tr("BIOS Manufacturer"), QString::fromStdString(bios.manufacturer));
    addRow(tr("BIOS Version"), QString::fromStdString(bios.version));
    addRow(tr("BIOS Date"), QString::fromStdString(bios.date));
    addRow(tr("BIOS Filename"), QString::fromStdString(bios.filename));
    
    // Resize rows to contents
    m_specsTable->resizeRowsToContents();
}

void MachineDialog::onMachineSelectionChanged(int index)
{
    if (index < 0) {
        return;
    }
    
    // Get selected machine
    QString machineName = m_machineCombo->itemText(index);
    auto& machineManager = MachineManager::getInstance();
    auto machine = machineManager.getMachineByName(machineName.toStdString());
    
    if (machine) {
        updateMachineDetails(machine);
    }
}

void MachineDialog::onOkClicked()
{
    QString machineName = m_machineCombo->currentText();
    
    // Update configuration
    m_config->setString("general", "machine", machineName.toStdString());
    
    // Set machine configuration
    auto& machineManager = MachineManager::getInstance();
    auto machine = machineManager.getMachineByName(machineName.toStdString());
    
    if (machine) {
        m_config->setMachineConfig(machine);
        m_config->applyMachineDefaults();
        
        Logger::GetInstance()->info("Selected machine: %s", machineName.toStdString().c_str());
    }
    
    accept();
}

void MachineDialog::onCancelClicked()
{
    reject();
}

} // namespace x86emu