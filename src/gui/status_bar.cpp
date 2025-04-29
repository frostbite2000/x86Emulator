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

#include "gui/status_bar.h"

EmulatorStatusBar::EmulatorStatusBar(QWidget* parent)
    : QStatusBar(parent),
      m_fpsLabel(nullptr),
      m_floppyALabel(nullptr),
      m_floppyBLabel(nullptr),
      m_cdromLabel(nullptr),
      m_hdLabel(nullptr),
      m_networkLabel(nullptr),
      m_floppyATimer(nullptr),
      m_floppyBTimer(nullptr),
      m_cdromTimer(nullptr),
      m_hdTimer(nullptr),
      m_networkTimer(nullptr)
{
    setupUi();
}

EmulatorStatusBar::~EmulatorStatusBar()
{
}

void EmulatorStatusBar::setupUi()
{
    // Load icons
    m_floppyInactiveIcon = QPixmap(":/icons/floppy_inactive.ico");
    m_floppyActiveIcon = QPixmap(":/icons/floppy_active.ico");
    m_floppyNoMediaIcon = QPixmap(":/icons/floppy_no_media.ico");
    m_cdromInactiveIcon = QPixmap(":/icons/cdrom_inactive.ico");
    m_cdromActiveIcon = QPixmap(":/icons/cdrom_active.ico");
    m_cdromNoMediaIcon = QPixmap(":/icons/cdrom_no_media.ico");
    m_hdInactiveIcon = QPixmap(":/icons/hd_inactive.ico");
    m_hdActiveIcon = QPixmap(":/icons/hd_active.ico");
    m_networkInactiveIcon = QPixmap(":/icons/network_inactive.ico");
    m_networkActiveIcon = QPixmap(":/icons/network_active.ico");
    
    // Create labels
    m_fpsLabel = new QLabel("FPS: 0.0", this);
    m_fpsLabel->setMinimumWidth(80);
    
    m_floppyALabel = new QLabel(this);
    m_floppyALabel->setPixmap(m_floppyNoMediaIcon);
    m_floppyALabel->setToolTip("Floppy Drive A:");
    
    m_floppyBLabel = new QLabel(this);
    m_floppyBLabel->setPixmap(m_floppyNoMediaIcon);
    m_floppyBLabel->setToolTip("Floppy Drive B:");
    
    m_cdromLabel = new QLabel(this);
    m_cdromLabel->setPixmap(m_cdromNoMediaIcon);
    m_cdromLabel->setToolTip("CD-ROM Drive");
    
    m_hdLabel = new QLabel(this);
    m_hdLabel->setPixmap(m_hdInactiveIcon);
    m_hdLabel->setToolTip("Hard Disk");
    
    m_networkLabel = new QLabel(this);
    m_networkLabel->setPixmap(m_networkInactiveIcon);
    m_networkLabel->setToolTip("Network");
    
    // Add labels to the status bar
    addPermanentWidget(m_networkLabel);
    addPermanentWidget(m_hdLabel);
    addPermanentWidget(m_cdromLabel);
    addPermanentWidget(m_floppyBLabel);
    addPermanentWidget(m_floppyALabel);
    addPermanentWidget(m_fpsLabel);
    
    // Create timers
    m_floppyATimer = new QTimer(this);
    m_floppyATimer->setSingleShot(true);
    m_floppyATimer->setInterval(250);
    connect(m_floppyATimer, &QTimer::timeout, this, &EmulatorStatusBar::onFloppyATimerTimeout);
    
    m_floppyBTimer = new QTimer(this);
    m_floppyBTimer->setSingleShot(true);
    m_floppyBTimer->setInterval(250);
    connect(m_floppyBTimer, &QTimer::timeout, this, &EmulatorStatusBar::onFloppyBTimerTimeout);
    
    m_cdromTimer = new QTimer(this);
    m_cdromTimer->setSingleShot(true);
    m_cdromTimer->setInterval(250);
    connect(m_cdromTimer, &QTimer::timeout, this, &EmulatorStatusBar::onCdromTimerTimeout);
    
    m_hdTimer = new QTimer(this);
    m_hdTimer->setSingleShot(true);
    m_hdTimer->setInterval(150);
    connect(m_hdTimer, &QTimer::timeout, this, &EmulatorStatusBar::onHdTimerTimeout);
    
    m_networkTimer = new QTimer(this);
    m_networkTimer->setSingleShot(true);
    m_networkTimer->setInterval(250);
    connect(m_networkTimer, &QTimer::timeout, this, &EmulatorStatusBar::onNetworkTimerTimeout);
}

void EmulatorStatusBar::updateStatus(Emulator* emulator)
{
    if (!emulator) {
        return;
    }
    
    // Check for drive activity
    if (emulator->getFloppyActivity(0)) {
        setFloppyActivity(0, true);
    }
    
    if (emulator->getFloppyActivity(1)) {
        setFloppyActivity(1, true);
    }
    
    if (emulator->getCDROMActivity()) {
        setCDROMActivity(true);
    }
    
    if (emulator->getHardDiskActivity(0)) {
        setHardDiskActivity(0, true);
    }
    
    if (emulator->getNetworkActivity()) {
        setNetworkActivity(true);
    }
}

void EmulatorStatusBar::setFps(double fps)
{
    m_fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
}

void EmulatorStatusBar::setFloppyActivity(int drive, bool active)
{
    if (drive == 0) {
        if (active) {
            m_floppyALabel->setPixmap(m_floppyActiveIcon);
            m_floppyATimer->start();
        }
    } else if (drive == 1) {
        if (active) {
            m_floppyBLabel->setPixmap(m_floppyActiveIcon);
            m_floppyBTimer->start();
        }
    }
}

void EmulatorStatusBar::setFloppyMounted(int drive, bool mounted)
{
    if (drive == 0) {
        m_floppyALabel->setPixmap(mounted ? m_floppyInactiveIcon : m_floppyNoMediaIcon);
    } else if (drive == 1) {
        m_floppyBLabel->setPixmap(mounted ? m_floppyInactiveIcon : m_floppyNoMediaIcon);
    }
}

void EmulatorStatusBar::setCDROMActivity(bool active)
{
    if (active) {
        m_cdromLabel->setPixmap(m_cdromActiveIcon);
        m_cdromTimer->start();
    }
}

void EmulatorStatusBar::setCDROMMounted(bool mounted)
{
    m_cdromLabel->setPixmap(mounted ? m_cdromInactiveIcon : m_cdromNoMediaIcon);
}

void EmulatorStatusBar::setHardDiskActivity(int drive, bool active)
{
    Q_UNUSED(drive);
    
    if (active) {
        m_hdLabel->setPixmap(m_hdActiveIcon);
        m_hdTimer->start();
    }
}

void EmulatorStatusBar::setNetworkActivity(bool active)
{
    if (active) {
        m_networkLabel->setPixmap(m_networkActiveIcon);
        m_networkTimer->start();
    }
}

void EmulatorStatusBar::onFloppyATimerTimeout()
{
    // Check if there's media in the drive
    if (m_floppyALabel->pixmap().cacheKey() == m_floppyActiveIcon.cacheKey()) {
        m_floppyALabel->setPixmap(m_floppyInactiveIcon);
    }
}

void EmulatorStatusBar::onFloppyBTimerTimeout()
{
    // Check if there's media in the drive
    if (m_floppyBLabel->pixmap().cacheKey() == m_floppyActiveIcon.cacheKey()) {
        m_floppyBLabel->setPixmap(m_floppyInactiveIcon);
    }
}

void EmulatorStatusBar::onCdromTimerTimeout()
{
    // Check if there's media in the drive
    if (m_cdromLabel->pixmap().cacheKey() == m_cdromActiveIcon.cacheKey()) {
        m_cdromLabel->setPixmap(m_cdromInactiveIcon);
    }
}

void EmulatorStatusBar::onHdTimerTimeout()
{
    m_hdLabel->setPixmap(m_hdInactiveIcon);
}

void EmulatorStatusBar::onNetworkTimerTimeout()
{
    m_networkLabel->setPixmap(m_networkInactiveIcon);
}