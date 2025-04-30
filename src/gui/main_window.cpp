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

#include "gui/main_window.h"
#include "gui/settings_dialog.h"
#include "gui/display_area.h"
#include "gui/status_bar.h"
#include "gui/toolbar.h"
#include "gui/about_dialog.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QScreen>
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QPixmap>
#include <QImage>
#include <QImageWriter>
#include <QStandardPaths>
#include <QDateTime>
#include <QShortcut>
#include <QSettings>

// Maximum number of recent files to remember
const int MAX_RECENT_FILES = 5;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_emulator(nullptr),
      m_configManager(nullptr),
      m_displayArea(nullptr),
      m_statusBar(nullptr),
      m_toolBar(nullptr),
      m_emulationTimer(nullptr),
      m_frameCount(0),
      m_fps(0.0),
      m_emulationRunning(false),
      m_emulationPaused(false),
      m_captureInput(false),
      m_scaleFactor(1)
{
    // Set window properties
    setWindowTitle("x86Emulator");
    setWindowIcon(QIcon(":/icons/app_icon.ico"));
    
    // Accept drops
    setAcceptDrops(true);
    
    // Create emulator instance
    m_emulator = new Emulator();
    m_configManager = m_emulator->getConfigManager();
    
    // Initialize the UI
    setupUi();
    
    // Load settings
    loadSettings();
    
    // Show the window
    show();
}

MainWindow::~MainWindow()
{
    // Stop emulation if running
    if (m_emulationRunning) {
        stopEmulation();
    }
    
    // Save settings
    saveSettings();
    
    // Clean up
    delete m_emulator;
}

void MainWindow::initialize()
{
    // Initialize emulator
    if (!m_emulator->initialize()) {
        QMessageBox::critical(this, "Error", "Failed to initialize emulator");
        QApplication::quit();
        return;
    }
    
    // Connect emulator signals
    
    // Initialize display area
    m_displayArea->initialize(m_emulator);
    
    // Update the status bar
    updateStatusBar();
    updateWindowTitle();
    
    // Update the actions
    updateActions();
}

void MainWindow::startEmulation()
{
    if (m_emulationRunning) {
        return;
    }
    
    // Start the emulator
    if (!m_emulator->start()) {
        QMessageBox::critical(this, "Error", "Failed to start emulation");
        return;
    }
    
    // Start the emulation timer
    m_emulationTimer->start();
    
    // Update state variables
    m_emulationRunning = true;
    m_emulationPaused = false;
    
    // Update UI elements
    updateActions();
    updateStatusBar();
    updateWindowTitle();
    
    // Reset frame counting
    m_lastFrameTime = QDateTime::currentDateTime();
    m_frameCount = 0;
    m_fps = 0.0;
}

void MainWindow::pauseEmulation()
{
    if (!m_emulationRunning || m_emulationPaused) {
        return;
    }
    
    // Pause the emulator
    m_emulator->pause();
    
    // Update state variables
    m_emulationPaused = true;
    
    // Update UI elements
    updateActions();
    updateStatusBar();
    updateWindowTitle();
}

void MainWindow::resumeEmulation()
{
    if (!m_emulationRunning || !m_emulationPaused) {
        return;
    }
    
    // Resume the emulator
    m_emulator->resume();
    
    // Update state variables
    m_emulationPaused = false;
    
    // Update UI elements
    updateActions();
    updateStatusBar();
    updateWindowTitle();
}

void MainWindow::stopEmulation()
{
    if (!m_emulationRunning) {
        return;
    }
    
    // Stop the emulator
    m_emulator->stop();
    
    // Stop the emulation timer
    m_emulationTimer->stop();
    
    // Update state variables
    m_emulationRunning = false;
    m_emulationPaused = false;
    
    // Update UI elements
    updateActions();
    updateStatusBar();
    updateWindowTitle();
}

void MainWindow::updateDisplay()
{
    // Update the display area
    m_displayArea->update();
    
    // Update the status bar periodically
    static int updateCounter = 0;
    if (++updateCounter >= 30) { // Update every 30 frames
        updateStatusBar();
        updateCounter = 0;
    }
    
    // Update FPS counter
    m_frameCount++;
    updateFpsCounter();
}

void MainWindow::handleKeyPress(QKeyEvent* event)
{
    if (!m_captureInput) {
        return;
    }
    
    // Let the emulator handle the key press
    m_emulator->keyPressed(event->key(), event->modifiers());
    
    // Prevent the event from being processed further
    event->accept();
}

void MainWindow::handleKeyRelease(QKeyEvent* event)
{
    if (!m_captureInput) {
        return;
    }
    
    // Let the emulator handle the key release
    m_emulator->keyReleased(event->key(), event->modifiers());
    
    // Prevent the event from being processed further
    event->accept();
}

void MainWindow::setFullScreen(bool fullscreen)
{
    if (fullscreen) {
        // Save the current window state
        m_configManager->setInt("ui", "window_state", (int)windowState());
        
        // Hide menus and status bar in full screen mode
        menuBar()->hide();
        m_toolBar->hide();
        m_statusBar->hide();
        
        // Enter full screen mode
        showFullScreen();
    } else {
        // Show menus and status bar
        menuBar()->show();
        m_toolBar->show();
        m_statusBar->show();
        
        // Restore previous window state
        int state = m_configManager->getInt("ui", "window_state", (int)Qt::WindowNoState);
        setWindowState((Qt::WindowState)state);
    }
    
    // Update actions
    m_actionFullScreen->setChecked(fullscreen);
    
    // Update the display area
    m_displayArea->setFullScreen(fullscreen);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Confirm exit if emulation is running
    if (m_emulationRunning) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
            "Exit Confirmation",
            "Emulation is still running. Do you want to exit?",
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    
    // Save settings
    saveSettings();
    
    // Stop emulation
    if (m_emulationRunning) {
        stopEmulation();
    }
    
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    
    // Update the display area if needed
    if (m_displayArea) {
        m_displayArea->adjustSize();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    // Check if the drag contains file paths
    if (event->mimeData()->hasUrls()) {
        bool hasValidFile = false;
        
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QFileInfo fileInfo(filePath);
                
                // Accept known file types
                QString extension = fileInfo.suffix().toLower();
                if (extension == "img" || extension == "ima" || extension == "vfd" || // floppy images
                    extension == "iso" || extension == "cue" || extension == "bin" || // CD-ROM images
                    extension == "hdi" || extension == "hdd" || extension == "vhd") { // hard disk images
                    hasValidFile = true;
                    break;
                }
            }
        }
        
        if (hasValidFile) {
            event->acceptProposedAction();
        }
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            QFileInfo fileInfo(filePath);
            
            // Handle dropped file based on extension
            QString extension = fileInfo.suffix().toLower();
            
            if (extension == "img" || extension == "ima" || extension == "vfd") {
                // Ask which floppy drive to use
                QMessageBox msgBox;
                msgBox.setText("Select floppy drive:");
                msgBox.addButton("Drive A:", QMessageBox::AcceptRole);
                msgBox.addButton("Drive B:", QMessageBox::AcceptRole);
                msgBox.addButton("Cancel", QMessageBox::RejectRole);
                
                int ret = msgBox.exec();
                if (ret == 0) {
                    mountFloppyDisk(0, filePath);
                } else if (ret == 1) {
                    mountFloppyDisk(1, filePath);
                }
            } else if (extension == "iso" || extension == "cue" || extension == "bin") {
                mountCDROM(filePath);
            } else if (extension == "hdi" || extension == "hdd" || extension == "vhd") {
                // Handle hard disk images
                // This would require more complex handling
                QMessageBox::information(this, "Not Implemented", "Hard disk image mounting via drag and drop is not yet implemented.");
            }
        }
    }
    
    event->acceptProposedAction();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    // Check for special key combinations first
    if (event->key() == Qt::Key_F11) {
        toggleFullScreen();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Escape && isFullScreen()) {
        setFullScreen(false);
        event->accept();
        return;
    } else if (event->modifiers() == (Qt::AltModifier | Qt::ControlModifier) && event->key() == Qt::Key_Delete) {
        if (m_emulationRunning) {
            m_emulator->sendCtrlAltDel();
        }
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F12) {
        takeScreenshot();
        event->accept();
        return;
    }
    
    // Forward other key events to the emulator
    if (m_captureInput && m_emulationRunning && !m_emulationPaused) {
        handleKeyPress(event);
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    // Forward key events to the emulator
    if (m_captureInput && m_emulationRunning && !m_emulationPaused) {
        handleKeyRelease(event);
    } else {
        QMainWindow::keyReleaseEvent(event);
    }
}

void MainWindow::onActionSettings()
{
    // Create and show the settings dialog
    SettingsDialog settingsDialog(m_emulator, this);
    
    // Pause emulation while in settings
    bool wasPaused = m_emulationPaused;
    if (m_emulationRunning && !m_emulationPaused) {
        pauseEmulation();
    }
    
    // Show dialog and process result
    int result = settingsDialog.exec();
    
    if (result == QDialog::Accepted) {
        // Apply settings
        m_configManager->saveConfiguration();
        
        // Check if restart is needed
        // ...
    }
    
    // Resume emulation if it was running before
    if (m_emulationRunning && !wasPaused) {
        resumeEmulation();
    }
}

void MainWindow::onActionExit()
{
    close();
}

void MainWindow::onActionReset()
{
    resetEmulator(false);
}

void MainWindow::onActionPause()
{
    if (m_emulationRunning) {
        if (m_emulationPaused) {
            resumeEmulation();
        } else {
            pauseEmulation();
        }
    }
}

void MainWindow::onActionHardReset()
{
    resetEmulator(true);
}

void MainWindow::onActionCtrlAltDel()
{
    if (m_emulationRunning) {
        m_emulator->sendCtrlAltDel();
    }
}

void MainWindow::onActionScreenshot()
{
    takeScreenshot();
}

void MainWindow::onActionFullScreen()
{
    toggleFullScreen();
}

void MainWindow::onActionAbout()
{
    AboutDialog aboutDialog(this);
    aboutDialog.exec();
}

void MainWindow::onActionFloppyA()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    
    QString tag = action->data().toString();
    if (tag == "eject") {
        ejectFloppyDisk(0);
    } else if (tag == "select") {
        QString filter = "Floppy disk images (*.img *.ima *.vfd);;All files (*.*)";
        QString filePath = QFileDialog::getOpenFileName(this, "Select Floppy Disk Image for Drive A:", "", filter);
        
        if (!filePath.isEmpty()) {
            mountFloppyDisk(0, filePath);
        }
    } else {
        // It's a recent file
        mountFloppyDisk(0, tag);
    }
}

void MainWindow::onActionFloppyB()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    
    QString tag = action->data().toString();
    if (tag == "eject") {
        ejectFloppyDisk(1);
    } else if (tag == "select") {
        QString filter = "Floppy disk images (*.img *.ima *.vfd);;All files (*.*)";
        QString filePath = QFileDialog::getOpenFileName(this, "Select Floppy Disk Image for Drive B:", "", filter);
        
        if (!filePath.isEmpty()) {
            mountFloppyDisk(1, filePath);
        }
    } else {
        // It's a recent file
        mountFloppyDisk(1, tag);
    }
}

void MainWindow::onActionCDROM()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    
    QString tag = action->data().toString();
    if (tag == "eject") {
        ejectCDROM();
    } else if (tag == "select") {
        QString filter = "CD/DVD images (*.iso *.cue *.bin *.img);;All files (*.*)";
        QString filePath = QFileDialog::getOpenFileName(this, "Select CD/DVD Image", "", filter);
        
        if (!filePath.isEmpty()) {
            mountCDROM(filePath);
        }
    } else {
        // It's a recent file
        mountCDROM(tag);
    }
}

void MainWindow::updateStatusBar()
{
    if (m_statusBar) {
        // Update status bar indicators
        m_statusBar->updateStatus(m_emulator);
        
        // Update FPS display
        m_statusBar->setFps(m_fps);
    }
}

void MainWindow::updateWindowTitle()
{
    QString title = "x86Emulator";
    
    // Add emulation status if needed
    if (m_emulationRunning) {
        if (m_emulationPaused) {
            title += " [Paused]";
        }
        
        // Add machine info
        QString machineType = m_configManager->getString("machine", "model", "");
        if (!machineType.isEmpty()) {
            title += " - " + machineType;
        }
    }
    
    setWindowTitle(title);
}

void MainWindow::onEmulationTimer()
{
    if (m_emulationRunning && !m_emulationPaused) {
        // Run one frame
        m_emulator->runFrame();
        
        // Update the display
        updateDisplay();
    }
}

void MainWindow::setupUi()
{
    // Create main UI components
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupDisplayArea();
    setupShortcuts();
    setupConnections();
    
    // Create actions
    createActions();
    
    // Create emulation timer
    m_emulationTimer = new QTimer(this);
    m_emulationTimer->setInterval(1000 / 60); // 60 FPS
    connect(m_emulationTimer, &QTimer::timeout, this, &MainWindow::onEmulationTimer);
    
    // Set central widget
    setCentralWidget(m_displayArea);
    
    // Set initial size
    resize(800, 600);
}

void MainWindow::setupMenuBar()
{
    // Create menus
    m_actionMenu = menuBar()->addMenu("Action");
    m_viewMenu = menuBar()->addMenu("View");
    m_mediaMenu = menuBar()->addMenu("Media");
    m_toolsMenu = menuBar()->addMenu("Tools");
    m_helpMenu = menuBar()->addMenu("Help");
    
    // Create submenu for floppy drives
    m_floppyAMenu = m_mediaMenu->addMenu("Floppy A:");
    m_floppyBMenu = m_mediaMenu->addMenu("Floppy B:");
    m_cdromMenu = m_mediaMenu->addMenu("CD-ROM");
}

void MainWindow::setupToolBar()
{
    m_toolBar = new EmulatorToolBar(this);
    addToolBar(Qt::TopToolBarArea, m_toolBar);
    m_toolBar->setMovable(false);
    m_toolBar->setFloatable(false);
}

void MainWindow::setupStatusBar()
{
    m_statusBar = new EmulatorStatusBar(this);
    setStatusBar(m_statusBar);
}

void MainWindow::setupDisplayArea()
{
    m_displayArea = new DisplayArea(this);
    m_displayArea->setFocusPolicy(Qt::StrongFocus);
    m_displayArea->setMouseTracking(true);
}

void MainWindow::setupShortcuts()
{
    // F11 for fullscreen
    QShortcut* fullscreenShortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(fullscreenShortcut, &QShortcut::activated, this, &MainWindow::toggleFullScreen);
    
    // F12 for screenshot
    QShortcut* screenshotShortcut = new QShortcut(QKeySequence(Qt::Key_F12), this);
    connect(screenshotShortcut, &QShortcut::activated, this, &MainWindow::takeScreenshot);
    
    // Ctrl+Alt+Delete
    QShortcut* ctrlAltDelShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Delete), this);
    connect(ctrlAltDelShortcut, &QShortcut::activated, this, &MainWindow::onActionCtrlAltDel);
    
    // Ctrl+R for reset
    QShortcut* resetShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), this);
    connect(resetShortcut, &QShortcut::activated, this, &MainWindow::onActionReset);
    
    // Ctrl+P for pause
    QShortcut* pauseShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_P), this);
    connect(pauseShortcut, &QShortcut::activated, this, &MainWindow::onActionPause);
}

void MainWindow::setupConnections()
{
    // Connect toolbar signals
    connect(m_toolBar, &EmulatorToolBar::settingsClicked, this, &MainWindow::onActionSettings);
    connect(m_toolBar, &EmulatorToolBar::resetClicked, this, &MainWindow::onActionReset);
    connect(m_toolBar, &EmulatorToolBar::pauseClicked, this, &MainWindow::onActionPause);
    connect(m_toolBar, &EmulatorToolBar::powerClicked, this, &MainWindow::onActionHardReset);
    connect(m_toolBar, &EmulatorToolBar::fullscreenClicked, this, &MainWindow::toggleFullScreen);
    
    // Connect display area signals
    connect(m_displayArea, &DisplayArea::mouseStateChanged, this, [this](bool captured) {
        m_captureInput = captured;
    });
}

void MainWindow::showMachineDialog()
{
    MachineDialog dialog(this, m_configManager);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Update UI based on selected machine
        updateUIForMachine();
    }
}

void MainWindow::updateUIForMachine()
{
    // Get current machine
    auto machine = m_configManager->getMachineConfig();
    if (!machine) {
        return;
    }
    
    // Update window title
    setWindowTitle(QString("x86Emulator - %1").arg(QString::fromStdString(machine->getName())));
    
    // Update status bar
    m_statusMachineLabel->setText(QString("Machine: %1").arg(QString::fromStdString(machine->getName())));
    m_statusChipsetLabel->setText(QString("Chipset: %1").arg(QString::fromStdString(machine->getChipsetName())));
    
    // Update available options in settings dialogs
    // (This would typically be handled by change callbacks on the config)
    
    // Check if emulator is running and needs restart
    if (m_emulator && m_emulator->isRunning()) {
        QMessageBox::information(this, tr("Machine Changed"), 
                              tr("The machine type has been changed. Please restart the emulator for the changes to take effect."));
    }
}

void MainWindow::createActions()
{
    // Action menu
    m_actionSettings = m_actionMenu->addAction("Settings...", this, &MainWindow::onActionSettings, QKeySequence(Qt::CTRL | Qt::Key_S));
    m_actionMenu->addSeparator();
    m_actionReset = m_actionMenu->addAction("Reset", this, &MainWindow::onActionReset, QKeySequence(Qt::CTRL | Qt::Key_R));
    m_actionHardReset = m_actionMenu->addAction("Hard Reset", this, &MainWindow::onActionHardReset);
    m_actionPause = m_actionMenu->addAction("Pause", this, &MainWindow::onActionPause, QKeySequence(Qt::CTRL | Qt::Key_P));
    m_actionCtrlAltDel = m_actionMenu->addAction("Ctrl+Alt+Del", this, &MainWindow::onActionCtrlAltDel);
    m_actionMenu->addSeparator();
    m_actionExit = m_actionMenu->addAction("Exit", this, &MainWindow::onActionExit, QKeySequence(Qt::CTRL | Qt::Key_Q));
    
    // Machine action
    m_machineAction = new QAction(tr("Select &Machine..."), this);
    m_machineAction->setStatusTip(tr("Select machine type"));
    connect(m_machineAction, &QAction::triggered, this, &MainWindow::showMachineDialog);
    
    // Add to Machine menu
    m_machineMenu->addAction(m_machineAction);
    
    // View menu
    m_actionFullScreen = m_viewMenu->addAction("Fullscreen", this, &MainWindow::toggleFullScreen, QKeySequence(Qt::Key_F11));
    m_actionFullScreen->setCheckable(true);
    m_actionFullScreen->setChecked(false);
    m_actionScreenshot = m_viewMenu->addAction("Screenshot", this, &MainWindow::takeScreenshot, QKeySequence(Qt::Key_F12));
    
    // Floppy A menu
    m_actionFloppyAEject = m_floppyAMenu->addAction("Eject", this, &MainWindow::onActionFloppyA);
    m_actionFloppyAEject->setData("eject");
    m_floppyAMenu->addSeparator();
    m_actionFloppyASelect = m_floppyAMenu->addAction("Select Disk Image...", this, &MainWindow::onActionFloppyA);
    m_actionFloppyASelect->setData("select");
    m_floppyAMenu->addSeparator();
    
    // Add recent floppy A images
    for (int i = 0; i < MAX_RECENT_FILES; ++i) {
        QAction* action = m_floppyAMenu->addAction("", this, &MainWindow::onActionFloppyA);
        action->setVisible(false);
        m_recentFloppyAActions.append(action);
    }
    
    // Floppy B menu
    m_actionFloppyBEject = m_floppyBMenu->addAction("Eject", this, &MainWindow::onActionFloppyB);
    m_actionFloppyBEject->setData("eject");
    m_floppyBMenu->addSeparator();
    m_actionFloppyBSelect = m_floppyBMenu->addAction("Select Disk Image...", this, &MainWindow::onActionFloppyB);
    m_actionFloppyBSelect->setData("select");
    m_floppyBMenu->addSeparator();
    
    // Add recent floppy B images
    for (int i = 0; i < MAX_RECENT_FILES; ++i) {
        QAction* action = m_floppyBMenu->addAction("", this, &MainWindow::onActionFloppyB);
        action->setVisible(false);
        m_recentFloppyBActions.append(action);
    }
    
    // CD-ROM menu
    m_actionCDROMEject = m_cdromMenu->addAction("Eject", this, &MainWindow::onActionCDROM);
    m_actionCDROMEject->setData("eject");
    m_cdromMenu->addSeparator();
    m_actionCDROMSelect = m_cdromMenu->addAction("Select Disk Image...", this, &MainWindow::onActionCDROM);
    m_actionCDROMSelect->setData("select");
    m_cdromMenu->addSeparator();
    
    // Add recent CD-ROM images
    for (int i = 0; i < MAX_RECENT_FILES; ++i) {
        QAction* action = m_cdromMenu->addAction("", this, &MainWindow::onActionCDROM);
        action->setVisible(false);
        m_recentCDROMActions.append(action);
    }
    
    // Help menu
    m_actionAbout = m_helpMenu->addAction("About...", this, &MainWindow::onActionAbout);
    
    // Add actions to toolbar
    m_toolBar->addAction(m_actionSettings);
    m_toolBar->addAction(m_actionReset);
    m_toolBar->addAction(m_actionPause);
    m_toolBar->addAction(m_actionFullScreen);
    
    // Update action states
    updateActions();
}

void MainWindow::updateActions()
{
    bool running = m_emulationRunning;
    bool paused = m_emulationPaused;
    
    // Update action states
    m_actionReset->setEnabled(running);
    m_actionHardReset->setEnabled(running);
    m_actionPause->setEnabled(running);
    m_actionPause->setText(paused ? "Resume" : "Pause");
    m_actionCtrlAltDel->setEnabled(running && !paused);
    m_actionScreenshot->setEnabled(running);
    
    // Update toolbar buttons
    m_toolBar->setPauseButtonState(running, paused);
    
    // Update media actions
    bool mediaEnabled = running && !paused;
    m_actionFloppyAEject->setEnabled(mediaEnabled);
    m_actionFloppyASelect->setEnabled(mediaEnabled);
    m_actionFloppyBEject->setEnabled(mediaEnabled);
    m_actionFloppyBSelect->setEnabled(mediaEnabled);
    m_actionCDROMEject->setEnabled(mediaEnabled);
    m_actionCDROMSelect->setEnabled(mediaEnabled);
    
    // Update recent file actions
    for (auto action : m_recentFloppyAActions) {
        action->setEnabled(mediaEnabled);
    }
    for (auto action : m_recentFloppyBActions) {
        action->setEnabled(mediaEnabled);
    }
    for (auto action : m_recentCDROMActions) {
        action->setEnabled(mediaEnabled);
    }
}

void MainWindow::loadSettings()
{
    // Load window settings
    int width = m_configManager->getInt("ui", "window_width", 800);
    int height = m_configManager->getInt("ui", "window_height", 600);
    bool fullscreen = m_configManager->getBool("ui", "fullscreen", false);
    m_scaleFactor = m_configManager->getInt("ui", "scale_factor", 1);
    
    // Apply window settings
    resize(width, height);
    if (fullscreen) {
        setFullScreen(true);
    }
    
    // Load recent files
    m_recentFloppyA = m_configManager->getStringList("recent", "floppy_a", QStringList());
    m_recentFloppyB = m_configManager->getStringList("recent", "floppy_b", QStringList());
    m_recentCDROM = m_configManager->getStringList("recent", "cdrom", QStringList());
    
    // Update recent file actions
    for (int i = 0; i < m_recentFloppyA.size() && i < MAX_RECENT_FILES; ++i) {
        m_recentFloppyAActions[i]->setText(QFileInfo(m_recentFloppyA[i]).fileName());
        m_recentFloppyAActions[i]->setData(m_recentFloppyA[i]);
        m_recentFloppyAActions[i]->setVisible(true);
    }
    
    for (int i = 0; i < m_recentFloppyB.size() && i < MAX_RECENT_FILES; ++i) {
        m_recentFloppyBActions[i]->setText(QFileInfo(m_recentFloppyB[i]).fileName());
        m_recentFloppyBActions[i]->setData(m_recentFloppyB[i]);
        m_recentFloppyBActions[i]->setVisible(true);
    }
    
    for (int i = 0; i < m_recentCDROM.size() && i < MAX_RECENT_FILES; ++i) {
        m_recentCDROMActions[i]->setText(QFileInfo(m_recentCDROM[i]).fileName());
        m_recentCDROMActions[i]->setData(m_recentCDROM[i]);
        m_recentCDROMActions[i]->setVisible(true);
    }
}

void MainWindow::saveSettings()
{
    // Save window settings (only if not in fullscreen)
    if (!isFullScreen()) {
        m_configManager->setInt("ui", "window_width", width());
        m_configManager->setInt("ui", "window_height", height());
    }
    
    m_configManager->setBool("ui", "fullscreen", isFullScreen());
    m_configManager->setInt("ui", "scale_factor", m_scaleFactor);
    
    // Save recent files
    m_configManager->setStringList("recent", "floppy_a", m_recentFloppyA);
    m_configManager->setStringList("recent", "floppy_b", m_recentFloppyB);
    m_configManager->setStringList("recent", "cdrom", m_recentCDROM);
    
    // Save configuration
    m_configManager->saveConfiguration();
}

void MainWindow::ejectFloppyDisk(int drive)
{
    if (!m_emulationRunning || m_emulationPaused) {
        return;
    }
    
    // Eject disk from emulator
    m_emulator->ejectFloppyDisk(drive);
    
    // Update status bar
    m_statusBar->setFloppyActivity(drive, false);
    m_statusBar->setFloppyMounted(drive, false);
    
    // Update the display
    updateDisplay();
}

void MainWindow::mountFloppyDisk(int drive, const QString& path)
{
    if (!m_emulationRunning || m_emulationPaused) {
        return;
    }
    
    // Mount disk in emulator
    if (m_emulator->mountFloppyDisk(drive, path.toStdString())) {
        // Update status bar
        m_statusBar->setFloppyMounted(drive, true);
        
        // Add to recent files
        QStringList& recentList = (drive == 0) ? m_recentFloppyA : m_recentFloppyB;
        QList<QAction*>& actions = (drive == 0) ? m_recentFloppyAActions : m_recentFloppyBActions;
        
        // Remove if already in list
        recentList.removeAll(path);
        
        // Add to the beginning
        recentList.prepend(path);
        
        // Keep only the maximum number
        while (recentList.size() > MAX_RECENT_FILES) {
            recentList.removeLast();
        }
        
        // Update actions
        for (int i = 0; i < recentList.size() && i < MAX_RECENT_FILES; ++i) {
            actions[i]->setText(QFileInfo(recentList[i]).fileName());
            actions[i]->setData(recentList[i]);
            actions[i]->setVisible(true);
        }
        
        // Update the display
        updateDisplay();
    } else {
        QMessageBox::warning(this, "Mount Failed", 
            QString("Failed to mount floppy image '%1' on drive %2")
                .arg(path)
                .arg(drive == 0 ? "A:" : "B:"));
    }
}

void MainWindow::ejectCDROM()
{
    if (!m_emulationRunning || m_emulationPaused) {
        return;
    }
    
    // Eject CD-ROM from emulator
    m_emulator->ejectCDROM();
    
    // Update status bar
    m_statusBar->setCDROMActivity(false);
    m_statusBar->setCDROMMounted(false);
    
    // Update the display
    updateDisplay();
}

void MainWindow::mountCDROM(const QString& path)
{
    if (!m_emulationRunning || m_emulationPaused) {
        return;
    }
    
    // Mount CD-ROM in emulator
    if (m_emulator->mountCDROM(path.toStdString())) {
        // Update status bar
        m_statusBar->setCDROMMounted(true);
        
        // Add to recent files
        m_recentCDROM.removeAll(path);
        m_recentCDROM.prepend(path);
        
        // Keep only the maximum number
        while (m_recentCDROM.size() > MAX_RECENT_FILES) {
            m_recentCDROM.removeLast();
        }
        
        // Update actions
        for (int i = 0; i < m_recentCDROM.size() && i < MAX_RECENT_FILES; ++i) {
            m_recentCDROMActions[i]->setText(QFileInfo(m_recentCDROM[i]).fileName());
            m_recentCDROMActions[i]->setData(m_recentCDROM[i]);
            m_recentCDROMActions[i]->setVisible(true);
        }
        
        // Update the display
        updateDisplay();
    } else {
        QMessageBox::warning(this, "Mount Failed", 
            QString("Failed to mount CD-ROM image '%1'").arg(path));
    }
}

void MainWindow::takeScreenshot()
{
    if (!m_emulationRunning) {
        return;
    }
    
    // Get the current display image
    QImage screenshot = m_displayArea->grabFramebuffer();
    
    // Create screenshots directory if it doesn't exist
    QString directory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/x86Emulator";
    QDir dir(directory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Generate a timestamped filename
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString filename = QString("%1/screenshot_%2.ico").arg(directory).arg(timestamp);
    
    // Save the screenshot
    if (screenshot.save(filename)) {
        // Show brief notification in the status bar
        m_statusBar->showMessage("Screenshot saved: " + filename, 3000);
    } else {
        QMessageBox::warning(this, "Screenshot Failed", 
            "Failed to save screenshot to " + filename);
    }
}

void MainWindow::resetEmulator(bool hard)
{
    if (!m_emulationRunning) {
        return;
    }
    
    // Reset the emulator
    if (hard) {
        m_emulator->hardReset();
    } else {
        m_emulator->reset();
    }
    
    // Unpause if paused
    if (m_emulationPaused) {
        resumeEmulation();
    }
    
    // Update UI
    updateActions();
    updateStatusBar();
}

void MainWindow::updateFpsCounter()
{
    // Update FPS once per second
    QDateTime currentTime = QDateTime::currentDateTime();
    qint64 elapsed = m_lastFrameTime.msecsTo(currentTime);
    
    if (elapsed >= 1000) {
        m_fps = (m_frameCount * 1000.0) / elapsed;
        m_frameCount = 0;
        m_lastFrameTime = currentTime;
        
        // Update status bar
        m_statusBar->setFps(m_fps);
    }
}