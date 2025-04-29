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

#ifndef SOUND_SETTINGS_PAGE_H
#define SOUND_SETTINGS_PAGE_H

#include "gui/settings_dialog.h"

/**
 * @brief The Sound settings page for configuring sound cards and audio output
 */
class SoundSettingsPage : public SettingsPage {
    Q_OBJECT
public:
    explicit SoundSettingsPage(Emulator* emulator, QWidget* parent = nullptr);
    ~SoundSettingsPage() override;

    void apply() override;
    void load() override;
    void reset() override;
    bool hasChanges() const override;

private slots:
    void onSoundCardChanged(int index);
    void onMidiDeviceChanged(int index);
    void onConfigureCardClicked();

private:
    void setupUi();
    void populateSoundCards();
    void populateMidiDevices();
    void updateCardDependentControls();
    
    Emulator* m_emulator;
    
    // Original values for change detection
    QString m_origSoundCard;
    QString m_origMidiDevice;
    int m_origVolume;
    bool m_origMute;
    
    // UI Controls
    QComboBox* m_soundCardCombo;
    QPushButton* m_configureCardButton;
    QComboBox* m_midiDeviceCombo;
    QSlider* m_volumeSlider;
    QCheckBox* m_muteCheck;
    
    // Sound card configuration data
    QHash<QString, QStringList> m_soundCardIrqs;
    QHash<QString, QStringList> m_soundCardDmas;
};

#endif // SOUND_SETTINGS_PAGE_H