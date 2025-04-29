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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QTimer>
#include <QIcon>
#include <QToolBar>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QPixmap>
#include <QPainter>
#include <QDateTime>

#include "emulator.h"
#include "display_area.h"
#include "status_bar.h"
#include "toolbar.h"
#include "config_manager.h"

// Forward declarations
class SettingsDialog;
class DisplayArea;
class EmulatorStatusBar;
class EmulatorToolBar;

/**
 * @brief Main application window for the x86Emulator
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void initialize();
    void startEmulation();
    void pauseEmulation();
    void resumeEmulation();
    void stopEmulation();
    bool isEmulationRunning() const { return m_emulationRunning; }
    bool isEmulationPaused() const { return m_emulationPaused; }

public slots:
    void updateDisplay();
    void handleKeyPress(QKeyEvent* event);
    void handleKeyRelease(QKeyEvent* event);
    void setFullScreen(bool fullscreen);
    void toggleFullScreen() { setFullScreen(!isFullScreen()); }

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    // Action handlers
    void onActionSettings();
    void onActionExit();
    void onActionReset();
    void onActionPause();
    void onActionHardReset();
    void onActionCtrlAltDel();
    void onActionScreenshot();
    void onActionFullScreen();
    void onActionAbout();
    
    // Media menu actions
    void onActionFloppyA();
    void onActionFloppyB();
    void onActionCDROM();
    
    // Status update handlers
    void updateStatusBar();
    void updateWindowTitle();
    
    // Timer related slots
    void onEmulationTimer();

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupDisplayArea();
    void setupShortcuts();
    void setupConnections();
    
    void createActions();
    void updateActions();
    
    void loadSettings();
    void saveSettings();
    
    // Helper methods
    void ejectFloppyDisk(int drive);
    void mountFloppyDisk(int drive, const QString& path);
    void ejectCDROM();
    void mountCDROM(const QString& path);
    void takeScreenshot();
    void resetEmulator(bool hard);
    
    void updateFpsCounter();
    
    // Private member variables
    Emulator* m_emulator;
    ConfigManager* m_configManager;
    
    DisplayArea* m_displayArea;
    EmulatorStatusBar* m_statusBar;
    EmulatorToolBar* m_toolBar;
    
    QTimer* m_emulationTimer;
    QDateTime m_lastFrameTime;
    int m_frameCount;
    double m_fps;
    
    bool m_emulationRunning;
    bool m_emulationPaused;
    bool m_captureInput;
    
    // Actions
    QAction* m_actionSettings;
    QAction* m_actionExit;
    QAction* m_actionReset;
    QAction* m_actionHardReset;
    QAction* m_actionPause;
    QAction* m_actionCtrlAltDel;
    QAction* m_actionFullScreen;
    QAction* m_actionScreenshot;
    QAction* m_actionAbout;
    
    // Media actions
    QAction* m_actionFloppyAEject;
    QAction* m_actionFloppyASelect;
    QAction* m_actionFloppyBEject;
    QAction* m_actionFloppyBSelect;
    QAction* m_actionCDROMEject;
    QAction* m_actionCDROMSelect;
    
    // Menus
    QMenu* m_actionMenu;
    QMenu* m_viewMenu;
    QMenu* m_mediaMenu;
    QMenu* m_toolsMenu;
    QMenu* m_helpMenu;
    QMenu* m_floppyAMenu;
    QMenu* m_floppyBMenu;
    QMenu* m_cdromMenu;
    
    // Recent media lists
    QStringList m_recentFloppyA;
    QStringList m_recentFloppyB;
    QStringList m_recentCDROM;
    QList<QAction*> m_recentFloppyAActions;
    QList<QAction*> m_recentFloppyBActions;
    QList<QAction*> m_recentCDROMActions;
    
    // Screen scaling
    int m_scaleFactor;
};

#endif // MAIN_WINDOW_H