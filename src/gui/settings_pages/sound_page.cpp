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

#include "gui/settings_pages/sound_page.h"
#include "config_manager.h"
#include "emulator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

SoundSettingsPage::SoundSettingsPage(Emulator* emulator, QWidget* parent)
    : SettingsPage(parent),
      m_emulator(emulator),
      m_origVolume(100),
      m_origMute(false)
{
    setupUi();
    
    populateSoundCards();
    populateMidiDevices();
    updateCardDependentControls();
}

SoundSettingsPage::~SoundSettingsPage()
{
}

void SoundSettingsPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create form layout
    QFormLayout* formLayout = createFormLayout();
    
    // Sound card combobox with Configure button
    QHBoxLayout* soundCardLayout = new QHBoxLayout();
    m_soundCardCombo = new QComboBox(this);
    connect(m_soundCardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SoundSettingsPage::onSoundCardChanged);
    soundCardLayout->addWidget(m_soundCardCombo);
    
    m_configureCardButton = new QPushButton(tr("Configure"), this);
    m_configureCardButton->setFixedWidth(100);
    connect(m_configureCardButton, &QPushButton::clicked, this, &SoundSettingsPage::onConfigureCardClicked);
    soundCardLayout->addWidget(m_configureCardButton);
    
    formLayout->addRow(tr("Sound card:"), soundCardLayout);
    
    // MIDI device combobox
    m_midiDeviceCombo = new QComboBox(this);
    connect(m_midiDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SoundSettingsPage::onMidiDeviceChanged);
    formLayout->addRow(tr("MIDI device:"), m_midiDeviceCombo);
    
    // Volume control group
    QGroupBox* volumeGroup = new QGroupBox(tr("Volume control"), this);
    QVBoxLayout* volumeLayout = new QVBoxLayout(volumeGroup);
    
    // Volume slider with label
    QHBoxLayout* sliderLayout = new QHBoxLayout();
    QLabel* minLabel = new QLabel(tr("Min"), volumeGroup);
    sliderLayout->addWidget(minLabel);
    
    m_volumeSlider = new QSlider(Qt::Horizontal, volumeGroup);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setTickInterval(10);
    m_volumeSlider->setTickPosition(QSlider::TicksBelow);
    sliderLayout->addWidget(m_volumeSlider, 1);
    
    QLabel* maxLabel = new QLabel(tr("Max"), volumeGroup);
    sliderLayout->addWidget(maxLabel);
    
    volumeLayout->addLayout(sliderLayout);
    
    // Mute checkbox
    m_muteCheck = new QCheckBox(tr("Mute"), volumeGroup);
    volumeLayout->addWidget(m_muteCheck);
    
    formLayout->addRow("", volumeGroup);
    
    // Add form layout to main layout
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
}

void SoundSettingsPage::populateSoundCards()
{
    m_soundCardCombo->clear();
    m_soundCardCombo->addItem(tr("None"));
    m_soundCardCombo->addItem(tr("PC Speaker"));
    m_soundCardCombo->addItem(tr("Tandy"));
    m_soundCardCombo->addItem(tr("Adlib"));
    m_soundCardCombo->addItem(tr("Sound Blaster 1.0"));
    m_soundCardCombo->addItem(tr("Sound Blaster 1.5"));
    m_soundCardCombo->addItem(tr("Sound Blaster 2.0"));
    m_soundCardCombo->addItem(tr("Sound Blaster Pro"));
    m_soundCardCombo->addItem(tr("Sound Blaster 16"));
    m_soundCardCombo->addItem(tr("Sound Blaster AWE32"));
    m_soundCardCombo->addItem(tr("Gravis Ultrasound"));
    m_soundCardCombo->addItem(tr("Creative Ensoniq AudioPCI"));
}

void SoundSettingsPage::populateMidiDevices()
{
    m_midiDeviceCombo->clear();
    m_midiDeviceCombo->addItem(tr("None"));
    m_midiDeviceCombo->addItem(tr("Internal MIDI"));
    m_midiDeviceCombo->addItem(tr("FluidSynth"));
    m_midiDeviceCombo->addItem(tr("Roland MT-32"));
    m_midiDeviceCombo->addItem(tr("System MIDI"));
}

void SoundSettingsPage::updateCardDependentControls()
{
    // Enable/disable the configure button based on the selected sound card
    QString card = m_soundCardCombo->currentText();
    bool configurable = !(card == "None" || card == "PC Speaker");
    m_configureCardButton->setEnabled(configurable);
    
    // Update MIDI devices based on sound card selection
    // For example, some sound cards might have built-in MIDI
}

void SoundSettingsPage::onSoundCardChanged(int index)
{
    Q_UNUSED(index);
    updateCardDependentControls();
}

void SoundSettingsPage::onMidiDeviceChanged(int index)
{
    Q_UNUSED(index);
    // Handle MIDI device changes if needed
}

void SoundSettingsPage::onConfigureCardClicked()
{
    QMessageBox::information(this, tr("Sound Card Configuration"), 
                           tr("This would open a sound card-specific configuration dialog."));
}

void SoundSettingsPage::load()
{
    ConfigManager* config = m_emulator->getConfigManager();
    
    // Load sound card
    m_origSoundCard = config->getString("sound", "card", "Sound Blaster 16");
    int soundCardIndex = m_soundCardCombo->findText(m_origSoundCard);
    if (soundCardIndex >= 0) {
        m_soundCardCombo->setCurrentIndex(soundCardIndex);
    }
    
    // Load MIDI device
    m_origMidiDevice = config->getString("sound", "midi_device", "FluidSynth");
    int midiDeviceIndex = m_midiDeviceCombo->findText(m_origMidiDevice);
    if (midiDeviceIndex >= 0) {
        m_midiDeviceCombo->setCurrentIndex(midiDeviceIndex);
    }
    
    // Load volume
    m_origVolume = config->getInt("sound", "volume", 100);
    m_volumeSlider->setValue(m_origVolume);
    
    // Load mute
    m_origMute = config->getBool("sound", "mute", false);
    m_muteCheck->setChecked(m_origMute);
}

void SoundSettingsPage::apply()
{
    ConfigManager* config = m_emulator->getConfigManager();
    
    // Save sound card
    config->setString("sound", "card", m_soundCardCombo->currentText());
    
    // Save MIDI device
    config->setString("sound", "midi_device", m_midiDeviceCombo->currentText());
    
    // Save volume
    config->setInt("sound", "volume", m_volumeSlider->value());
    
    // Save mute
    config->setBool("sound", "mute", m_muteCheck->isChecked());
}

void SoundSettingsPage::reset()
{
    // Reset to defaults
    int soundCardIndex = m_soundCardCombo->findText("Sound Blaster 16");
    if (soundCardIndex >= 0) {
        m_soundCardCombo->setCurrentIndex(soundCardIndex);
    }
    
    int midiDeviceIndex = m_midiDeviceCombo->findText("FluidSynth");
    if (midiDeviceIndex >= 0) {
        m_midiDeviceCombo->setCurrentIndex(midiDeviceIndex);
    }
    
    m_volumeSlider->setValue(100);
    m_muteCheck->setChecked(false);
}

bool SoundSettingsPage::hasChanges() const
{
    // Check if any settings have changed
    if (m_soundCardCombo->currentText() != m_origSoundCard) return true;
    if (m_midiDeviceCombo->currentText() != m_origMidiDevice) return true;
    if (m_volumeSlider->value() != m_origVolume) return true;
    if (m_muteCheck->isChecked() != m_origMute) return true;
    
    return false;
}