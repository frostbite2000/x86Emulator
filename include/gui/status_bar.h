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

#ifndef EMULATOR_STATUS_BAR_H
#define EMULATOR_STATUS_BAR_H

#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTimer>
#include <QList>

#include "emulator.h"

/**
 * @brief Custom status bar for the emulator with drive activity indicators
 */
class EmulatorStatusBar : public QStatusBar {
    Q_OBJECT
public:
    explicit EmulatorStatusBar(QWidget* parent = nullptr);
    ~EmulatorStatusBar() override;
    
    void updateStatus(Emulator* emulator);
    void setFps(double fps);
    
    // Drive status methods
    void setFloppyActivity(int drive, bool active);
    void setFloppyMounted(int drive, bool mounted);
    void setCDROMActivity(bool active);
    void setCDROMMounted(bool mounted);
    void setHardDiskActivity(int drive, bool active);
    void setNetworkActivity(bool active);
    
private:
    void setupUi();
    
    // Status indicators
    QLabel* m_fpsLabel;
    QLabel* m_floppyALabel;
    QLabel* m_floppyBLabel;
    QLabel* m_cdromLabel;
    QLabel* m_hdLabel;
    QLabel* m_networkLabel;
    
    // Activity timers
    QTimer* m_floppyATimer;
    QTimer* m_floppyBTimer;
    QTimer* m_cdromTimer;
    QTimer* m_hdTimer;
    QTimer* m_networkTimer;
    
    // Icons for different states
    QPixmap m_floppyInactiveIcon;
    QPixmap m_floppyActiveIcon;
    QPixmap m_floppyNoMediaIcon;
    QPixmap m_cdromInactiveIcon;
    QPixmap m_cdromActiveIcon;
    QPixmap m_cdromNoMediaIcon;
    QPixmap m_hdInactiveIcon;
    QPixmap m_hdActiveIcon;
    QPixmap m_networkInactiveIcon;
    QPixmap m_networkActiveIcon;

private slots:
    void onFloppyATimerTimeout();
    void onFloppyBTimerTimeout();
    void onCdromTimerTimeout();
    void onHdTimerTimeout();
    void onNetworkTimerTimeout();
};

#endif // EMULATOR_STATUS_BAR_H