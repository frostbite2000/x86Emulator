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

#ifndef X86EMULATOR_MACHINE_DIALOG_H
#define X86EMULATOR_MACHINE_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <memory>

namespace x86emu {

// Forward declarations
class ConfigManager;
class MachineConfig;

/**
 * @brief Machine selection dialog
 * 
 * This dialog allows the user to select a machine type.
 */
class MachineDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct a new MachineDialog
     * 
     * @param parent Parent widget
     * @param config Configuration manager
     */
    explicit MachineDialog(QWidget *parent, std::shared_ptr<ConfigManager> config);
    
    /**
     * @brief Destroy the MachineDialog
     */
    ~MachineDialog() override;

private slots:
    /**
     * @brief Handle machine selection change
     * 
     * @param index Selected index
     */
    void onMachineSelectionChanged(int index);
    
    /**
     * @brief Handle OK button click
     */
    void onOkClicked();
    
    /**
     * @brief Handle Cancel button click
     */
    void onCancelClicked();

private:
    std::shared_ptr<ConfigManager> m_config;
    
    QComboBox* m_machineCombo;
    QLabel* m_descriptionLabel;
    QTextEdit* m_descriptionText;
    QTableWidget* m_specsTable;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    
    /**
     * @brief Initialize the dialog
     */
    void initializeDialog();
    
    /**
     * @brief Update machine details
     * 
     * @param machine Machine configuration
     */
    void updateMachineDetails(std::shared_ptr<MachineConfig> machine);
    
    /**
     * @brief Populate the dialog with available machines
     */
    void populateMachines();
};

} // namespace x86emu

#endif // X86EMULATOR_MACHINE_DIALOG_H