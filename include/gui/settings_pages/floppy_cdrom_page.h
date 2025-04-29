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

#ifndef FLOPPY_CDROM_PAGE_H
#define FLOPPY_CDROM_PAGE_H

#include "gui/settings_dialog.h"

/**
 * @brief The Floppy & CD-ROM drives settings page
 */
class FloppyCdromSettingsPage : public SettingsPage {
    Q_OBJECT
public:
    explicit FloppyCdromSettingsPage(Emulator* emulator, QWidget* parent = nullptr);
    ~FloppyCdromSettingsPage() override;

    void apply() override;
    void load() override;
    void reset() override;
    bool hasChanges() const override;

private slots:
    void onFloppyATypeChanged(int index);
    void onFloppyBTypeChanged(int index);
    void onCdromTypeChanged(int index);
    void onBrowseFloppyAClicked();
    void onBrowseFloppyBClicked();
    void onBrowseCdromClicked();

private:
    void setupUi();
    void populateFloppyTypes();
    void populateCdromTypes();
    
    Emulator* m_emulator;
    
    // Original values for change detection
    QString m_origFloppyAType;
    QString m_origFloppyBType;
    QString m_origCdromType;
    QString m_origFloppyAPath;
    QString m_origFloppyBPath;
    QString m_origCdromPath;
    bool m_origTurboTimings;
    bool m_origCheckBPB;
    
    // UI Controls
    QTabWidget* m_tabWidget;
    
    // Floppy A Tab
    QWidget* m_floppyATab;
    QComboBox* m_floppyATypeCombo;
    QLineEdit* m_floppyAPathEdit;
    QPushButton* m_floppyABrowseButton;
    QCheckBox* m_floppyATurboCheck;
    QCheckBox* m_floppyACheckBPBCheck;
    
    // Floppy B Tab
    QWidget* m_floppyBTab;
    QComboBox* m_floppyBTypeCombo;
    QLineEdit* m_floppyBPathEdit;
    QPushButton* m_floppyBBrowseButton;
    QCheckBox* m_floppyBTurboCheck;
    QCheckBox* m_floppyBCheckBPBCheck;
    
    // CD-ROM Tab
    QWidget* m_cdromTab;
    QComboBox* m_cdromTypeCombo;
    QLineEdit* m_cdromPathEdit;
    QPushButton* m_cdromBrowseButton;
};

#endif // FLOPPY_CDROM_PAGE_H