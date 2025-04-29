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

#include "gui/settings_pages/machine_page.h"
#include "config_manager.h"
#include "emulator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

MachineSettingsPage::MachineSettingsPage(Emulator* emulator, QWidget* parent)
    : SettingsPage(parent),
      m_emulator(emulator),
      m_origFrequency(0),
      m_origMemory(0),
      m_origDynamicRecompiler(false),
      m_origSoftfloatFpu(false),
      m_origTimeSyncMode(0)
{
    setupUi();
    
    // Setup machine configuration data
    populateMachineTypes();
    populateMachines();
    populateCpuTypes();
    populateFrequencies();
    populateFpus();
    populateWaitStates();
    populatePitModes();
    populateMemorySizes();
}

MachineSettingsPage::~MachineSettingsPage()
{
}

void MachineSettingsPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create form layout
    QFormLayout* formLayout = createFormLayout();
    
    // Machine Type combobox
    m_machineTypeCombo = new QComboBox(this);
    connect(m_machineTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MachineSettingsPage::onMachineTypeChanged);
    formLayout->addRow(tr("Machine type:"), m_machineTypeCombo);
    
    // Machine combobox with Configure button
    QHBoxLayout* machineLayout = new QHBoxLayout();
    m_machineCombo = new QComboBox(this);
    connect(m_machineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MachineSettingsPage::onMachineChanged);
    machineLayout->addWidget(m_machineCombo);
    
    m_configureButton = new QPushButton(tr("Configure"), this);
    m_configureButton->setFixedWidth(100);
    connect(m_configureButton, &QPushButton::clicked, this, &MachineSettingsPage::onConfigureClicked);
    machineLayout->addWidget(m_configureButton);
    
    formLayout->addRow(tr("Machine:"), machineLayout);
    
    // CPU Type combobox
    m_cpuTypeCombo = new QComboBox(this);
    connect(m_cpuTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MachineSettingsPage::onCpuTypeChanged);
    formLayout->addRow(tr("CPU type:"), m_cpuTypeCombo);
    
    // CPU Frequency combobox
    m_frequencyCombo = new QComboBox(this);
    formLayout->addRow(tr("Frequency:"), m_frequencyCombo);
    
    // FPU combobox
    m_fpuCombo = new QComboBox(this);
    formLayout->addRow(tr("FPU:"), m_fpuCombo);
    
    // Wait States combobox
    m_waitStateCombo = new QComboBox(this);
    formLayout->addRow(tr("Wait states:"), m_waitStateCombo);
    
    // PIT Mode combobox
    m_pitModeCombo = new QComboBox(this);
    formLayout->addRow(tr("PIT mode:"), m_pitModeCombo);
    
    // Memory combobox
    m_memoryCombo = new QComboBox(this);
    formLayout->addRow(tr("Memory:"), m_memoryCombo);
    
    // Checkboxes
    m_dynamicRecompilerCheck = new QCheckBox(tr("Dynamic Recompiler"), this);
    formLayout->addRow("", m_dynamicRecompilerCheck);
    
    m_softfloatFpuCheck = new QCheckBox(tr("Softfloat FPU"), this);
    formLayout->addRow("", m_softfloatFpuCheck);
    
    // Time Synchronization group box
    m_timeSyncGroup = new QGroupBox(tr("Time synchronization"), this);
    QVBoxLayout* timeSyncLayout = new QVBoxLayout(m_timeSyncGroup);
    
    m_disabledRadio = new QRadioButton(tr("Disabled"), m_timeSyncGroup);
    m_localTimeRadio = new QRadioButton(tr("Enabled (local time)"), m_timeSyncGroup);
    m_utcRadio = new QRadioButton(tr("Enabled (UTC)"), m_timeSyncGroup);
    
    m_timeSyncGroup_bg = new QButtonGroup(this);
    m_timeSyncGroup_bg->addButton(m_disabledRadio, 0);
    m_timeSyncGroup_bg->addButton(m_localTimeRadio, 1);
    m_timeSyncGroup_bg->addButton(m_utcRadio, 2);
    
    timeSyncLayout->addWidget(m_disabledRadio);
    timeSyncLayout->addWidget(m_localTimeRadio);
    timeSyncLayout->addWidget(m_utcRadio);
    
    formLayout->addRow("", m_timeSyncGroup);
    
    // Add form layout to main layout
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
}

void MachineSettingsPage::populateMachineTypes()
{
    m_machineTypeCombo->clear();
    m_machineTypeCombo->addItem(tr("[1979] 8088"));
    m_machineTypeCombo->addItem(tr("[1984] 286"));
    m_machineTypeCombo->addItem(tr("[1987] 386"));
    m_machineTypeCombo->addItem(tr("[1989] 486"));
    m_machineTypeCombo->addItem(tr("[1993] Pentium"));
    m_machineTypeCombo->addItem(tr("[1995] Pentium Pro"));
    m_machineTypeCombo->addItem(tr("[1997] Pentium II"));
    m_machineTypeCombo->addItem(tr("[1999] Pentium III"));
    m_machineTypeCombo->addItem(tr("[2000] Pentium 4"));
    
    // Setup machine lists by type
    m_machinesByType["[1979] 8088"] = {"[8088] IBM PC (1981)", "[8088] IBM XT (1983)", "[8088] Generic XT clone"};
    m_machinesByType["[1984] 286"] = {"[286] IBM AT (1984)", "[286] IBM XT Model 286", "[286] AMI 286", "[286] Award 286", "[286] Commodore PC 30 III"};
    m_machinesByType["[1987] 386"] = {"[386SX] IBM PS/2 Model 55SX", "[386DX] IBM PS/2 Model 80", "[386DX] AMI 386DX", "[386DX] Award 386DX"};
    m_machinesByType["[1989] 486"] = {"[486] AMI 486", "[486] Award 486", "[486] IBM PS/2 Model 70 (type 4)",
                                      "[486] Packard Bell PB410A", "[486] Intel Classic/PCI"};
    m_machinesByType["[1993] Pentium"] = {"[Socket 4] Intel Premiere/PCI", "[Socket 5] Intel Advanced/ZP", "[Socket 7] Award 430VX PCI", 
                                         "[Socket 7] Award 430TX PCI"};
    m_machinesByType["[1995] Pentium Pro"] = {"[Socket 8] Intel VS440FX", "[Slot 1] Intel VS440LX"};
    m_machinesByType["[1997] Pentium II"] = {"[Slot 1] Intel VS440BX", "[Slot 1] ASUS P2B-LS"};
    m_machinesByType["[1999] Pentium III"] = {"[Slot 1] ASUS P3B-F", "[Socket 370] ASUS CUSL2"};
    m_machinesByType["[2000] Pentium 4"] = {"[Socket 423] Intel D850GB", "[Socket 478] Intel D865GBF"};

    // Setup CPU lists by machine
    m_cpusByMachine["[8088] IBM PC (1981)"] = {"Intel 8088", "NEC V20"};
    m_cpusByMachine["[8088] IBM XT (1983)"] = {"Intel 8088", "NEC V20"};
    m_cpusByMachine["[8088] Generic XT clone"] = {"Intel 8088", "NEC V20"};
    m_cpusByMachine["[286] IBM AT (1984)"] = {"Intel 80286", "AMD 80286"};
    m_cpusByMachine["[286] IBM XT Model 286"] = {"Intel 80286", "AMD 80286"};
    m_cpusByMachine["[286] AMI 286"] = {"Intel 80286", "AMD 80286"};
    
    // Setup CPU frequencies
    m_frequenciesByCpu["Intel 8088"] = {4.77, 7.16, 8, 10};
    m_frequenciesByCpu["NEC V20"] = {4.77, 7.16, 8, 10, 12, 16};
    m_frequenciesByCpu["Intel 80286"] = {6, 8, 10, 12, 16, 20, 25};
}

void MachineSettingsPage::populateMachines()
{
    const QString& machineType = m_machineTypeCombo->currentText();
    m_machineCombo->clear();
    
    if (m_machinesByType.contains(machineType)) {
        const QStringList& machines = m_machinesByType[machineType];
        for (const QString& machine : machines) {
            m_machineCombo->addItem(machine);
        }
    }
}

void MachineSettingsPage::populateCpuTypes()
{
    const QString& machine = m_machineCombo->currentText();
    m_cpuTypeCombo->clear();
    
    if (m_cpusByMachine.contains(machine)) {
        const QStringList& cpus = m_cpusByMachine[machine];
        for (const QString& cpu : cpus) {
            m_cpuTypeCombo->addItem(cpu);
        }
    }
}

void MachineSettingsPage::populateFrequencies()
{
    const QString& cpu = m_cpuTypeCombo->currentText();
    m_frequencyCombo->clear();
    
    if (m_frequenciesByCpu.contains(cpu)) {
        const QList<int>& frequencies = m_frequenciesByCpu[cpu];
        for (int freq : frequencies) {
            m_frequencyCombo->addItem(QString::number(freq));
        }
    }
}

void MachineSettingsPage::populateFpus()
{
    m_fpuCombo->clear();
    m_fpuCombo->addItem(tr("None"));
    
    // Add FPU options based on CPU
    const QString& cpu = m_cpuTypeCombo->currentText();
    if (cpu.contains("8088") || cpu.contains("V20")) {
        m_fpuCombo->addItem(tr("8087"));
    } else if (cpu.contains("80286")) {
        m_fpuCombo->addItem(tr("287"));
    } else if (cpu.contains("386")) {
        m_fpuCombo->addItem(tr("387"));
    }
    
    // For CPUs with built-in FPU, disable this option
    if (cpu.contains("Pentium") || cpu.contains("486DX")) {
        m_fpuCombo->setEnabled(false);
    } else {
        m_fpuCombo->setEnabled(true);
    }
}

void MachineSettingsPage::populateWaitStates()
{
    m_waitStateCombo->clear();
    m_waitStateCombo->addItem(tr("Default"));
    m_waitStateCombo->addItem(tr("Off"));
    m_waitStateCombo->addItem(tr("On"));
    m_waitStateCombo->addItem(tr("Auto"));
}

void MachineSettingsPage::populatePitModes()
{
    m_pitModeCombo->clear();
    m_pitModeCombo->addItem(tr("Auto"));
    m_pitModeCombo->addItem(tr("Slowest"));
    m_pitModeCombo->addItem(tr("Slow"));
    m_pitModeCombo->addItem(tr("Standard"));
    m_pitModeCombo->addItem(tr("Fast"));
    m_pitModeCombo->addItem(tr("Faster"));
}

void MachineSettingsPage::populateMemorySizes()
{
    m_memoryCombo->clear();
    
    // Add memory sizes based on machine type
    const QString& machineType = m_machineTypeCombo->currentText();
    
    if (machineType.contains("[1979]")) {
        m_memoryCombo->addItem(tr("64 KB"));
        m_memoryCombo->addItem(tr("128 KB"));
        m_memoryCombo->addItem(tr("256 KB"));
        m_memoryCombo->addItem(tr("512 KB"));
        m_memoryCombo->addItem(tr("640 KB"));
    } else if (machineType.contains("[1984]")) {
        m_memoryCombo->addItem(tr("1 MB"));
        m_memoryCombo->addItem(tr("2 MB"));
        m_memoryCombo->addItem(tr("4 MB"));
        m_memoryCombo->addItem(tr("8 MB"));
        m_memoryCombo->addItem(tr("12 MB"));
        m_memoryCombo->addItem(tr("16 MB"));
    } else if (machineType.contains("[1987]") || machineType.contains("[1989]")) {
        m_memoryCombo->addItem(tr("1 MB"));
        m_memoryCombo->addItem(tr("2 MB"));
        m_memoryCombo->addItem(tr("4 MB"));
        m_memoryCombo->addItem(tr("8 MB"));
        m_memoryCombo->addItem(tr("16 MB"));
        m_memoryCombo->addItem(tr("32 MB"));
        m_memoryCombo->addItem(tr("64 MB"));
    } else if (machineType.contains("[1993]") || machineType.contains("[1995]")) {
        m_memoryCombo->addItem(tr("4 MB"));
        m_memoryCombo->addItem(tr("8 MB"));
        m_memoryCombo->addItem(tr("16 MB"));
        m_memoryCombo->addItem(tr("32 MB"));
        m_memoryCombo->addItem(tr("64 MB"));
        m_memoryCombo->addItem(tr("128 MB"));
        m_memoryCombo->addItem(tr("256 MB"));
    } else {
        m_memoryCombo->addItem(tr("8 MB"));
        m_memoryCombo->addItem(tr("16 MB"));
        m_memoryCombo->addItem(tr("32 MB"));
        m_memoryCombo->addItem(tr("64 MB"));
        m_memoryCombo->addItem(tr("128 MB"));
        m_memoryCombo->addItem(tr("256 MB"));
        m_memoryCombo->addItem(tr("512 MB"));
        m_memoryCombo->addItem(tr("1 GB"));
        m_memoryCombo->addItem(tr("2 GB"));
    }
}

void MachineSettingsPage::updateMachineDependent()
{
    populateMachines();
    populateCpuTypes();
    populateFrequencies();
    populateFpus();
    populateMemorySizes();
    
    // Update configure button availability
    m_configureButton->setEnabled(!m_machineCombo->currentText().isEmpty());
}

void MachineSettingsPage::onMachineTypeChanged(int index)
{
    Q_UNUSED(index);
    updateMachineDependent();
}

void MachineSettingsPage::onMachineChanged(int index)
{
    Q_UNUSED(index);
    populateCpuTypes();
    populateFrequencies();
    populateFpus();
}

void MachineSettingsPage::onCpuTypeChanged(int index)
{
    Q_UNUSED(index);
    populateFrequencies();
    populateFpus();
}

void MachineSettingsPage::onConfigureClicked()
{
    QMessageBox::information(this, tr("Machine Configuration"), 
                           tr("This would open a machine-specific configuration dialog."));
}

void MachineSettingsPage::load()
{
    ConfigManager* config = m_emulator->getConfigManager();
    
    // Load machine type
    m_origMachineType = config->getString("machine", "type", "[1979] 8088");
    int machineTypeIndex = m_machineTypeCombo->findText(m_origMachineType);
    if (machineTypeIndex >= 0) {
        m_machineTypeCombo->setCurrentIndex(machineTypeIndex);
    }
    
    // Load machine
    m_origMachine = config->getString("machine", "model", "[8088] IBM PC (1981)");
    int machineIndex = m_machineCombo->findText(m_origMachine);
    if (machineIndex >= 0) {
        m_machineCombo->setCurrentIndex(machineIndex);
    }
    
    // Load CPU type
    m_origCpuType = config->getString("cpu", "type", "Intel 8088");
    int cpuTypeIndex = m_cpuTypeCombo->findText(m_origCpuType);
    if (cpuTypeIndex >= 0) {
        m_cpuTypeCombo->setCurrentIndex(cpuTypeIndex);
    }
    
    // Load CPU frequency
    m_origFrequency = config->getInt("cpu", "frequency", 4.77);
    int freqIndex = m_frequencyCombo->findText(QString::number(m_origFrequency));
    if (freqIndex >= 0) {
        m_frequencyCombo->setCurrentIndex(freqIndex);
    }
    
    // Load FPU
    m_origFpu = config->getString("cpu", "fpu", "None");
    int fpuIndex = m_fpuCombo->findText(m_origFpu);
    if (fpuIndex >= 0) {
        m_fpuCombo->setCurrentIndex(fpuIndex);
    }
    
    // Load wait states
    m_origWaitState = config->getString("cpu", "wait_states", "Default");
    int waitStateIndex = m_waitStateCombo->findText(m_origWaitState);
    if (waitStateIndex >= 0) {
        m_waitStateCombo->setCurrentIndex(waitStateIndex);
    }
    
    // Load PIT mode
    m_origPitMode = config->getString("timing", "pit_mode", "Auto");
    int pitModeIndex = m_pitModeCombo->findText(m_origPitMode);
    if (pitModeIndex >= 0) {
        m_pitModeCombo->setCurrentIndex(pitModeIndex);
    }
    
    // Load memory size
    m_origMemory = config->getInt("memory", "size", 640);
    QString memoryStr;
    if (m_origMemory >= 1024) {
        memoryStr = QString("%1 MB").arg(m_origMemory / 1024);
    } else {
        memoryStr = QString("%1 KB").arg(m_origMemory);
    }
    int memoryIndex = m_memoryCombo->findText(memoryStr);
    if (memoryIndex >= 0) {
        m_memoryCombo->setCurrentIndex(memoryIndex);
    }
    
    // Load dynamic recompiler setting
    m_origDynamicRecompiler = config->getBool("cpu", "dynamic_recompiler", false);
    m_dynamicRecompilerCheck->setChecked(m_origDynamicRecompiler);
    
    // Load softfloat FPU setting
    m_origSoftfloatFpu = config->getBool("cpu", "softfloat_fpu", false);
    m_softfloatFpuCheck->setChecked(m_origSoftfloatFpu);
    
    // Load time synchronization setting
    m_origTimeSyncMode = config->getInt("timing", "sync_mode", 1);
    switch (m_origTimeSyncMode) {
        case 0:
            m_disabledRadio->setChecked(true);
            break;
        case 1:
            m_localTimeRadio->setChecked(true);
            break;
        case 2:
            m_utcRadio->setChecked(true);
            break;
    }
}

void MachineSettingsPage::apply()
{
    ConfigManager* config = m_emulator->getConfigManager();
    
    // Save machine type
    config->setString("machine", "type", m_machineTypeCombo->currentText());
    
    // Save machine
    config->setString("machine", "model", m_machineCombo->currentText());
    
    // Save CPU type
    config->setString("cpu", "type", m_cpuTypeCombo->currentText());
    
    // Save CPU frequency
    double frequency = m_frequencyCombo->currentText().toDouble();
    config->setDouble("cpu", "frequency", frequency);
    
    // Save FPU
    config->setString("cpu", "fpu", m_fpuCombo->currentText());
    
    // Save wait states
    config->setString("cpu", "wait_states", m_waitStateCombo->currentText());
    
    // Save PIT mode
    config->setString("timing", "pit_mode", m_pitModeCombo->currentText());
    
    // Save memory size
    QString memoryStr = m_memoryCombo->currentText();
    int memSize = 0;
    if (memoryStr.contains("MB") || memoryStr.contains("GB")) {
        int multiplier = memoryStr.contains("GB") ? 1024 : 1;
        memSize = memoryStr.split(" ").first().toInt() * multiplier * 1024;
    } else {
        memSize = memoryStr.split(" ").first().toInt();
    }
    config->setInt("memory", "size", memSize);
    
    // Save dynamic recompiler setting
    config->setBool("cpu", "dynamic_recompiler", m_dynamicRecompilerCheck->isChecked());
    
    // Save softfloat FPU setting
    config->setBool("cpu", "softfloat_fpu", m_softfloatFpuCheck->isChecked());
    
    // Save time synchronization setting
    int timeSyncMode = m_timeSyncGroup_bg->checkedId();
    config->setInt("timing", "sync_mode", timeSyncMode);
}

void MachineSettingsPage::reset()
{
    // Reset to defaults
    m_machineTypeCombo->setCurrentIndex(0);
    m_machineCombo->setCurrentIndex(0);
    m_cpuTypeCombo->setCurrentIndex(0);
    m_frequencyCombo->setCurrentIndex(0);
    m_fpuCombo->setCurrentIndex(0);
    m_waitStateCombo->setCurrentIndex(0);
    m_pitModeCombo->setCurrentIndex(0);
    m_memoryCombo->setCurrentIndex(0);
    m_dynamicRecompilerCheck->setChecked(false);
    m_softfloatFpuCheck->setChecked(false);
    m_localTimeRadio->setChecked(true);
}

bool MachineSettingsPage::hasChanges() const
{
    // Check if any settings have changed
    if (m_machineTypeCombo->currentText() != m_origMachineType) return true;
    if (m_machineCombo->currentText() != m_origMachine) return true;
    if (m_cpuTypeCombo->currentText() != m_origCpuType) return true;
    if (m_frequencyCombo->currentText().toDouble() != m_origFrequency) return true;
    if (m_fpuCombo->currentText() != m_origFpu) return true;
    if (m_waitStateCombo->currentText() != m_origWaitState) return true;
    if (m_pitModeCombo->currentText() != m_origPitMode) return true;
    
    // Convert memory size for comparison
    QString memoryStr = m_memoryCombo->currentText();
    int memSize = 0;
    if (memoryStr.contains("MB") || memoryStr.contains("GB")) {
        int multiplier = memoryStr.contains("GB") ? 1024 : 1;
        memSize = memoryStr.split(" ").first().toInt() * multiplier * 1024;
    } else {
        memSize = memoryStr.split(" ").first().toInt();
    }
    if (memSize != m_origMemory) return true;
    
    if (m_dynamicRecompilerCheck->isChecked() != m_origDynamicRecompiler) return true;
    if (m_softfloatFpuCheck->isChecked() != m_origSoftfloatFpu) return true;
    if (m_timeSyncGroup_bg->checkedId() != m_origTimeSyncMode) return true;
    
    return false;
}