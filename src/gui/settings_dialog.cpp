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

#include "gui/settings_dialog.h"
#include "gui/settings_pages/machine_page.h"
#include "gui/settings_pages/display_page.h"
#include "gui/settings_pages/input_page.h"
#include "gui/settings_pages/sound_page.h"
#include "gui/settings_pages/network_page.h"
#include "gui/settings_pages/ports_page.h"
#include "gui/settings_pages/storage_controllers_page.h"
#include "gui/settings_pages/hard_disk_page.h"
#include "gui/settings_pages/floppy_cdrom_page.h"
#include "gui/settings_pages/other_removable_page.h"
#include "gui/settings_pages/other_peripherals_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QStyle>

SettingsDialog::SettingsDialog(Emulator* emulator, QWidget* parent)
    : QDialog(parent),
      m_emulator(emulator),
      m_configManager(emulator->getConfigManager())
{
    setWindowTitle(tr("Settings"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    // Set the dialog size to 80% of the screen size
    const QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
    int width = screenGeometry.width() * 0.8;
    int height = screenGeometry.height() * 0.8;
    resize(width, height);
    
    // Setup the UI
    setupUi();
    
    // Load settings
    loadSettings();
    
    // Select the first item by default
    if (m_categoriesList->count() > 0) {
        m_categoriesList->setCurrentRow(0);
    }
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUi()
{
    // Create main layout
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    
    // Create categories list
    m_categoriesList = new QListWidget(this);
    m_categoriesList->setIconSize(QSize(32, 32));
    m_categoriesList->setMinimumWidth(200);
    m_categoriesList->setMaximumWidth(250);
    connect(m_categoriesList, &QListWidget::currentItemChanged, this, &SettingsDialog::onCategoryChanged);
    
    // Create stacked widget for pages
    m_pagesStack = new QStackedWidget(this);
    
    // Create buttons
    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    
    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    connect(m_okButton, &QPushButton::clicked, this, &SettingsDialog::accept);
    
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::reject);
    
    buttonsLayout->addWidget(m_okButton);
    buttonsLayout->addWidget(m_cancelButton);
    
    // Create right panel layout
    QVBoxLayout* rightLayout = new QVBoxLayout;
    rightLayout->addWidget(m_pagesStack, 1);
    rightLayout->addLayout(buttonsLayout);
    
    // Add widgets to main layout
    mainLayout->addWidget(m_categoriesList);
    mainLayout->addLayout(rightLayout, 1);
    
    // Create settings pages
    m_machinePage = new MachineSettingsPage(m_emulator, this);
    m_displayPage = new DisplaySettingsPage(m_emulator, this);
    m_inputPage = new InputSettingsPage(m_emulator, this);
    m_soundPage = new SoundSettingsPage(m_emulator, this);
    m_networkPage = new NetworkSettingsPage(m_emulator, this);
    m_portsPage = new PortsSettingsPage(m_emulator, this);
    m_storageCtrlPage = new StorageControllerSettingsPage(m_emulator, this);
    m_hardDiskPage = new HardDiskSettingsPage(m_emulator, this);
    m_floppyCdromPage = new FloppyCdromSettingsPage(m_emulator, this);
    m_otherRemovablePage = new OtherRemovableSettingsPage(m_emulator, this);
    m_otherPeripheralsPage = new OtherPeripheralsSettingsPage(m_emulator, this);
    
    // Add pages
    addPage(m_machinePage, tr("Machine"), "machine");
    addPage(m_displayPage, tr("Display"), "display");
    addPage(m_inputPage, tr("Input devices"), "input");
    addPage(m_soundPage, tr("Sound"), "sound");
    addPage(m_networkPage, tr("Network"), "network");
    addPage(m_portsPage, tr("Ports (COM & LPT)"), "ports");
    addPage(m_storageCtrlPage, tr("Storage controllers"), "storage_controllers");
    addPage(m_hardDiskPage, tr("Hard disks"), "hard_disks");
    addPage(m_floppyCdromPage, tr("Floppy & CD-ROM drives"), "floppy_cdrom");
    addPage(m_otherRemovablePage, tr("Other removable devices"), "other_removable");
    addPage(m_otherPeripheralsPage, tr("Other peripherals"), "other_peripherals");
    
    // Apply style to list widget
    m_categoriesList->setStyleSheet(
        "QListWidget {"
        "    background-color: #2A2A2A;"
        "    border: none;"
        "}"
        "QListWidget::item {"
        "    padding: 10px;"
        "    border-radius: 0px;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #3A3A3A;"
        "    color: white;"
        "}"
    );
}

void SettingsDialog::loadSettings()
{
    // Load settings for each page
    for (SettingsPage* page : m_pages) {
        page->load();
    }
}

void SettingsDialog::applySettings()
{
    // Apply settings for each page
    for (SettingsPage* page : m_pages) {
        page->apply();
    }
}

void SettingsDialog::addPage(SettingsPage* page, const QString& title, const QString& iconName)
{
    // Add page to stack
    m_pagesStack->addWidget(page);
    
    // Add item to list
    QListWidgetItem* item = new QListWidgetItem(QIcon(QString(":/icons/%1.png").arg(iconName)), title);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_categoriesList->addItem(item);
    
    // Store page
    m_pages.append(page);
}

void SettingsDialog::onCategoryChanged(QListWidgetItem* current)
{
    if (current) {
        m_pagesStack->setCurrentIndex(m_categoriesList->row(current));
    }
}

void SettingsDialog::accept()
{
    // Check if any settings have changed
    bool hasChanges = false;
    for (SettingsPage* page : m_pages) {
        if (page->hasChanges()) {
            hasChanges = true;
            break;
        }
    }
    
    // If no changes, just close the dialog
    if (!hasChanges) {
        QDialog::accept();
        return;
    }
    
    // Apply settings
    applySettings();
    
    // Check if restart is required
    bool restartRequired = false;
    
    // Save settings
    m_configManager->saveConfiguration();
    
    // If restart required, show message
    if (restartRequired) {
        QMessageBox::information(this, tr("Restart Required"),
            tr("Some settings changes will only take effect after restarting the emulator."));
    }
    
    QDialog::accept();
}

void SettingsDialog::reject()
{
    // Check if any settings have changed
    bool hasChanges = false;
    for (SettingsPage* page : m_pages) {
        if (page->hasChanges()) {
            hasChanges = true;
            break;
        }
    }
    
    // If there are changes, confirm discard
    if (hasChanges) {
        QMessageBox::StandardButton response = QMessageBox::question(this, 
            tr("Discard Changes"), 
            tr("You have unsaved changes. Are you sure you want to discard them?"),
            QMessageBox::Yes | QMessageBox::No);
            
        if (response == QMessageBox::No) {
            return;
        }
    }
    
    QDialog::reject();
}