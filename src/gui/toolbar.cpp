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

#include "gui/toolbar.h"

EmulatorToolBar::EmulatorToolBar(QWidget* parent)
    : QToolBar(parent),
      m_settingsAction(nullptr),
      m_resetAction(nullptr),
      m_pauseAction(nullptr),
      m_powerAction(nullptr),
      m_fullscreenAction(nullptr)
{
    setupUi();
}

EmulatorToolBar::~EmulatorToolBar()
{
}

void EmulatorToolBar::setupUi()
{
    // Set toolbar properties
    setIconSize(QSize(24, 24));
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    
    // Load icons
    m_pauseIcon = QIcon(":/icons/pause.ico");
    m_resumeIcon = QIcon(":/icons/resume.ico");
    
    // Create actions
    m_settingsAction = addAction(QIcon(":/icons/settings.ico"), "Settings");
    m_resetAction = addAction(QIcon(":/icons/reset.ico"), "Reset");
    m_pauseAction = addAction(m_pauseIcon, "Pause");
    m_powerAction = addAction(QIcon(":/icons/power.ico"), "Power");
    m_fullscreenAction = addAction(QIcon(":/icons/fullscreen.ico"), "Fullscreen");
    
    // Connect actions
    connect(m_settingsAction, &QAction::triggered, this, &EmulatorToolBar::onSettingsActionTriggered);
    connect(m_resetAction, &QAction::triggered, this, &EmulatorToolBar::onResetActionTriggered);
    connect(m_pauseAction, &QAction::triggered, this, &EmulatorToolBar::onPauseActionTriggered);
    connect(m_powerAction, &QAction::triggered, this, &EmulatorToolBar::onPowerActionTriggered);
    connect(m_fullscreenAction, &QAction::triggered, this, &EmulatorToolBar::onFullscreenActionTriggered);
    
    // Set initial state
    m_resetAction->setEnabled(false);
    m_pauseAction->setEnabled(false);
    m_powerAction->setEnabled(false);
}

void EmulatorToolBar::setPauseButtonState(bool running, bool paused)
{
    m_pauseAction->setEnabled(running);
    
    if (paused) {
        m_pauseAction->setIcon(m_resumeIcon);
        m_pauseAction->setText("Resume");
    } else {
        m_pauseAction->setIcon(m_pauseIcon);
        m_pauseAction->setText("Pause");
    }
    
    m_resetAction->setEnabled(running);
    m_powerAction->setEnabled(running);
}

void EmulatorToolBar::onSettingsActionTriggered()
{
    emit settingsClicked();
}

void EmulatorToolBar::onResetActionTriggered()
{
    emit resetClicked();
}

void EmulatorToolBar::onPauseActionTriggered()
{
    emit pauseClicked();
}

void EmulatorToolBar::onPowerActionTriggered()
{
    emit powerClicked();
}

void EmulatorToolBar::onFullscreenActionTriggered()
{
    emit fullscreenClicked();
}