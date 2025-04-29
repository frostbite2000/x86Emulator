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

#ifndef EMULATOR_TOOLBAR_H
#define EMULATOR_TOOLBAR_H

#include <QToolBar>
#include <QAction>
#include <QIcon>

/**
 * @brief Custom toolbar for the emulator with quick access buttons
 */
class EmulatorToolBar : public QToolBar {
    Q_OBJECT
public:
    explicit EmulatorToolBar(QWidget* parent = nullptr);
    ~EmulatorToolBar() override;
    
    void setPauseButtonState(bool running, bool paused);

signals:
    void settingsClicked();
    void resetClicked();
    void pauseClicked();
    void powerClicked();
    void fullscreenClicked();

private slots:
    void onSettingsActionTriggered();
    void onResetActionTriggered();
    void onPauseActionTriggered();
    void onPowerActionTriggered();
    void onFullscreenActionTriggered();
    
private:
    void setupUi();
    
    QAction* m_settingsAction;
    QAction* m_resetAction;
    QAction* m_pauseAction;
    QAction* m_powerAction;
    QAction* m_fullscreenAction;
    
    QIcon m_pauseIcon;
    QIcon m_resumeIcon;
};

#endif // EMULATOR_TOOLBAR_H