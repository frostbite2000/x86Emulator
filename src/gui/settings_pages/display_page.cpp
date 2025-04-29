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

#include "gui/settings_pages/display_page.h"
#include "config_manager.h"
#include "emulator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

DisplaySettingsPage::DisplaySettingsPage(Emulator* emulator, QWidget* parent)
    : SettingsPage(parent),
      m_emulator(emulator),
      m_origVoodooEnabled(false),
      m_origVSync(false),
      m_origFullScreen(false),
      m_origHwCursor(false)
{
    setupUi();
    
    // Setup video card configuration data
    m_vramByCard["IBM VGA"] = {"256 KB"};
    m_vramByCard["Trident TVGA9000B"] = {"512 KB", "1 MB"};
    m_vramByCard["Tseng ET4000AX"] = {"512 KB", "1 MB"};
    m_vramByCard["S3 ViRGE/DX"] = {"2 MB", "4 MB"};
    m_vramByCard["S3 ViRGE/GX"] = {"2 MB", "4 MB"};
    m_vramByCard["S3 Trio64"] = {"1 MB", "2 MB"};
    m_vramByCard["NVIDIA GeForce 3 Ti 200"] = {"64 MB", "128 MB"};
    
    populateVideoCards();
    populateVoodooCards();
    populateRenderers();
    populateWindowScales();
    updateCardDependentControls();
}

DisplaySettingsPage::~DisplaySettingsPage()
{
}

void DisplaySettingsPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create form layout
    QFormLayout* formLayout = createFormLayout();
    
    // Video Card combobox with Configure button
    QHBoxLayout* videoCardLayout = new QHBoxLayout();
    m_videoCardCombo = new QComboBox(this);
    connect(m_videoCardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DisplaySettingsPage::onVideoCardChanged);
    videoCardLayout->addWidget(m_videoCardCombo);
    
    m_videoConfigureButton = new QPushButton(tr("Configure"), this);
    m_videoConfigureButton->setFixedWidth(100);
    connect(m_videoConfigureButton, &QPushButton::clicked, this, &DisplaySettingsPage::onVideoConfigureClicked);
    videoCardLayout->addWidget(m_videoConfigureButton);
    
    formLayout->addRow(tr("Video card:"), videoCardLayout);
    
    // Video RAM combobox
    m_videoRamCombo = new QComboBox(this);
    formLayout->addRow(tr("Video RAM:"), m_videoRamCombo);
    
    // 3dfx Voodoo group
    m_voodooGroup = new QGroupBox(tr("3dfx Voodoo Graphics"), this);
    QFormLayout* voodooLayout = new QFormLayout(m_voodooGroup);
    
    // Voodoo checkbox
    m_voodooCheck = new QCheckBox(tr("Enable 3dfx Voodoo Graphics"), m_voodooGroup);
    voodooLayout->addRow("", m_voodooCheck);
    
    // Voodoo model combo
    m_voodooModelCombo = new QComboBox(m_voodooGroup);
    connect(m_voodooModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DisplaySettingsPage::onVoodooModelChanged);
    voodooLayout->addRow(tr("Voodoo model:"), m_voodooModelCombo);
    
    formLayout->addRow("", m_voodooGroup);
    
    // General display options
    QGroupBox* displayOptionsGroup = new QGroupBox(tr("Display options"), this);
    QFormLayout* displayOptsLayout = new QFormLayout(displayOptionsGroup);
    
    // Renderer combobox
    m_rendererCombo = new QComboBox(displayOptionsGroup);
    displayOptsLayout->addRow(tr("Renderer:"), m_rendererCombo);
    
    // Window scale combobox
    m_windowScaleCombo = new QComboBox(displayOptionsGroup);
    displayOptsLayout->addRow(tr("Window scale:"), m_windowScaleCombo);
    
    // Display checkboxes
    m_vsyncCheck = new QCheckBox(tr("VSync"), displayOptionsGroup);
    displayOptsLayout->addRow("", m_vsyncCheck);
    
    m_fullScreenCheck = new QCheckBox(tr("Start in full screen"), displayOptionsGroup);
    displayOptsLayout->addRow("", m_fullScreenCheck);
    
    m_hwCursorCheck = new QCheckBox(tr("Hardware cursor"), displayOptionsGroup);
    displayOptsLayout->addRow("", m_hwCursorCheck);
    
    formLayout->addRow("", displayOptionsGroup);
    
    // Add form layout to main layout
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
}

void DisplaySettingsPage::populateVideoCards()
{
    m_videoCardCombo->clear();
    m_videoCardCombo->addItem(tr("IBM VGA"));
    m_videoCardCombo->addItem(tr("Trident TVGA9000B"));
    m_videoCardCombo->addItem(tr("Tseng ET4000AX"));
    m_videoCardCombo->addItem(tr("S3 ViRGE/DX"));
    m_videoCardCombo->addItem(tr("S3 ViRGE/GX"));
    m_videoCardCombo->addItem(tr("S3 Trio64"));
    m_videoCardCombo->addItem(tr("NVIDIA GeForce 3 Ti 200"));
}

void DisplaySettingsPage::populateVoodooCards()
{
    m_voodooModelCombo->clear();
    m_voodooModelCombo->addItem(tr("Voodoo 1 (4 MB)"));
    m_voodooModelCombo->addItem(tr("Voodoo 2 (8 MB)"));
    m_voodooModelCombo->addItem(tr("Voodoo 2 (12 MB)"));
    m_voodooModelCombo->addItem(tr("Voodoo Banshee (16 MB)"));
    m_voodooModelCombo->addItem(tr("Voodoo 3 2000 (16 MB)"));
    m_voodooModelCombo->addItem(tr("Voodoo 3 3000 (16 MB)"));
    m_voodooModelCombo->addItem(tr("Voodoo 3 5000 (16 MB)"));
}

void DisplaySettingsPage::populateRenderers()
{
    m_rendererCombo->clear();
    m_rendererCombo->addItem(tr("OpenGL"));
    m_rendererCombo->addItem(tr("Direct3D"));
    m_rendererCombo->addItem(tr("Software"));
}

void DisplaySettingsPage::populateWindowScales()
{
    m_windowScaleCombo->clear();
    m_windowScaleCombo->addItem(tr("0.5x"));
    m_windowScaleCombo->addItem(tr("1x"));
    m_windowScaleCombo->addItem(tr("1.5x"));
    m_windowScaleCombo->addItem(tr("2x"));
    m_windowScaleCombo->addItem(tr("3x"));
    m_windowScaleCombo->addItem(tr("4x"));
    m_windowScaleCombo->addItem(tr("5x"));
}

void DisplaySettingsPage::updateCardDependentControls()
{
    const QString& videoCard = m_videoCardCombo->currentText();
    
    // Update VRAM options
    m_videoRamCombo->clear();
    if (m_vramByCard.contains(videoCard)) {
        const QStringList& sizes = m_vramByCard[videoCard];
        for (const QString& size : sizes) {
            m_videoRamCombo->addItem(size);
        }
    }
    
    // Update Voodoo capability
    bool supportsVoodoo = !(videoCard.contains("GeForce") || videoCard.contains("Voodoo"));
    m_voodooGroup->setEnabled(supportsVoodoo);
    
    // Enable/disable Voodoo model based on checkbox state
    m_voodooModelCombo->setEnabled(m_voodooCheck->isChecked());
}

void DisplaySettingsPage::onVideoCardChanged(int index)
{
    Q_UNUSED(index);
    updateCardDependentControls();
}

void DisplaySettingsPage::onVideoConfigureClicked()
{
    QMessageBox::information(this, tr("Video Card Configuration"), 
                           tr("This would open a video card-specific configuration dialog."));
}

void DisplaySettingsPage::onVoodooModelChanged(int index)
{
    Q_UNUSED(index);
    // Handle any special logic based on Voodoo model
}

void DisplaySettingsPage::load()
{
    ConfigManager* config = m_emulator->getConfigManager();
    
    // Load video card
    m_origVideoCard = config->getString("video", "card", "IBM VGA");
    int videoCardIndex = m_videoCardCombo->findText(m_origVideoCard);
    if (videoCardIndex >= 0) {
        m_videoCardCombo->setCurrentIndex(videoCardIndex);
    }
    
    // Load video RAM
    m_origVideoRam = config->getString("video", "memory", "1 MB");
    int videoRamIndex = m_videoRamCombo->findText(m_origVideoRam);
    if (videoRamIndex >= 0) {
        m_videoRamCombo->setCurrentIndex(videoRamIndex);
    }
    
    // Load Voodoo settings
    m_origVoodooEnabled = config->getBool("voodoo", "enabled", false);
    m_voodooCheck->setChecked(m_origVoodooEnabled);
    
    m_origVoodooCard = config->getString("voodoo", "model", "Voodoo 1 (4 MB)");
    int voodooCardIndex = m_voodooModelCombo->findText(m_origVoodooCard);
    if (voodooCardIndex >= 0) {
        m_voodooModelCombo->setCurrentIndex(voodooCardIndex);
    }
    m_voodooModelCombo->setEnabled(m_origVoodooEnabled);
    
    // Load renderer
    m_origRenderer = config->getString("display", "renderer", "OpenGL");
    int rendererIndex = m_rendererCombo->findText(m_origRenderer);
    if (rendererIndex >= 0) {
        m_rendererCombo->setCurrentIndex(rendererIndex);
    }
    
    // Load window scale
    m_origWindowScale = config->getString("display", "scale", "1x");
    int windowScaleIndex = m_windowScaleCombo->findText(m_origWindowScale);
    if (windowScaleIndex >= 0) {
        m_windowScaleCombo->setCurrentIndex(windowScaleIndex);
    }
    
    // Load display options
    m_origVSync = config->getBool("display", "vsync", true);
    m_vsyncCheck->setChecked(m_origVSync);
    
    m_origFullScreen = config->getBool("display", "start_fullscreen", false);
    m_fullScreenCheck->setChecked(m_origFullScreen);
    
    m_origHwCursor = config->getBool("display", "hw_cursor", true);
    m_hwCursorCheck->setChecked(m_origHwCursor);
}

void DisplaySettingsPage::apply()
{
    ConfigManager* config = m_emulator->getConfigManager();
    
    // Save video card
    config->setString("video", "card", m_videoCardCombo->currentText());
    
    // Save video RAM
    config->setString("video", "memory", m_videoRamCombo->currentText());
    
    // Save Voodoo settings
    config->setBool("voodoo", "enabled", m_voodooCheck->isChecked());
    config->setString("voodoo", "model", m_voodooModelCombo->currentText());
    
    // Save renderer
    config->setString("display", "renderer", m_rendererCombo->currentText());
    
    // Save window scale
    config->setString("display", "scale", m_windowScaleCombo->currentText());
    
    // Save display options
    config->setBool("display", "vsync", m_vsyncCheck->isChecked());
    config->setBool("display", "start_fullscreen", m_fullScreenCheck->isChecked());
    config->setBool("display", "hw_cursor", m_hwCursorCheck->isChecked());
}

void DisplaySettingsPage::reset()
{
    // Reset to defaults
    m_videoCardCombo->setCurrentIndex(0);
    m_videoRamCombo->setCurrentIndex(0);
    m_voodooCheck->setChecked(false);
    m_voodooModelCombo->setCurrentIndex(0);
    m_voodooModelCombo->setEnabled(false);
    m_rendererCombo->setCurrentIndex(0);
    m_windowScaleCombo->setCurrentIndex(1); // 1x scale
    m_vsyncCheck->setChecked(true);
    m_fullScreenCheck->setChecked(false);
    m_hwCursorCheck->setChecked(true);
}

bool DisplaySettingsPage::hasChanges() const
{
    // Check if any settings have changed
    if (m_videoCardCombo->currentText() != m_origVideoCard) return true;
    if (m_videoRamCombo->currentText() != m_origVideoRam) return true;
    if (m_voodooCheck->isChecked() != m_origVoodooEnabled) return true;
    if (m_voodooModelCombo->currentText() != m_origVoodooCard) return true;
    if (m_rendererCombo->currentText() != m_origRenderer) return true;
    if (m_windowScaleCombo->currentText() != m_origWindowScale) return true;
    if (m_vsyncCheck->isChecked() != m_origVSync) return true;
    if (m_fullScreenCheck->isChecked() != m_origFullScreen) return true;
    if (m_hwCursorCheck->isChecked() != m_origHwCursor) return true;
    
    return false;
}