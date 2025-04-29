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

#ifndef MACHINE_SETTINGS_PAGE_H
#define MACHINE_SETTINGS_PAGE_H

#include "gui/settings_dialog.h"

/**
 * @brief The Machine settings page for configuring the emulated machine type and CPU
 */
class MachineSettingsPage : public SettingsPage {
    Q_OBJECT
public:
    explicit MachineSettingsPage(Emulator* emulator, QWidget* parent = nullptr);
    ~MachineSettingsPage() override;

    void apply() override;
    void load() override;
    void reset() override;
    bool hasChanges() const override;

private slots:
    void onMachineTypeChanged(int index);
    void onMachineChanged(int index);
    void onCpuTypeChanged(int index);
    void onConfigureClicked();

private:
    void setupUi();
    void populateMachineTypes();
    void populateMachines();
    void populateCpuTypes();
    void populateFrequencies();
    void populateFpus();
    void populateWaitStates();
    void populatePitModes();
    void populateMemorySizes();
    void updateMachineDependent();
    
    Emulator* m_emulator;
    
    // Original values for change detection
    QString m_origMachineType;
    QString m_origMachine;
    QString m_origCpuType;
    QString m_origFpu;
    int m_origFrequency;
    QString m_origWaitState;
    QString m_origPitMode;
    int m_origMemory;
    bool m_origDynamicRecompiler;
    bool m_origSoftfloatFpu;
    int m_origTimeSyncMode;
    
    // UI Controls
    QComboBox* m_machineTypeCombo;
    QComboBox* m_machineCombo;
    QPushButton* m_configureButton;
    QComboBox* m_cpuTypeCombo;
    QComboBox* m_frequencyCombo;
    QComboBox* m_fpuCombo;
    QComboBox* m_waitStateCombo;
    QComboBox* m_pitModeCombo;
    QComboBox* m_memoryCombo;
    QCheckBox* m_dynamicRecompilerCheck;
    QCheckBox* m_softfloatFpuCheck;
    
    // Time synchronization
    QGroupBox* m_timeSyncGroup;
    QRadioButton* m_disabledRadio;
    QRadioButton* m_localTimeRadio;
    QRadioButton* m_utcRadio;
    QButtonGroup* m_timeSyncGroup_bg;
    
    // Machine configuration data
    QHash<QString, QStringList> m_machinesByType;
    QHash<QString, QStringList> m_cpusByMachine;
    QHash<QString, QList<int>> m_frequenciesByCpu;
};

#endif // MACHINE_SETTINGS_PAGE_H