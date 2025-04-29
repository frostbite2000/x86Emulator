/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2025 frostbite2000
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

#ifndef X86EMULATOR_EMULATOR_H
#define X86EMULATOR_EMULATOR_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <QImage>
#include <QObject>

// Forward declarations
namespace x86emu {
    class X86CPU;
}

class ConfigManager;
class MemoryManager;
class IOManager;
class InterruptController;
class DeviceManager;
class TimerManager;
class GraphicsAdapter;
class SoundManager;
class FloppyDrive;
class CDROMDrive;
class HardDisk;
class NetworkAdapter;
class Logger;

/**
 * @brief The main emulator class that controls all emulation components.
 * 
 * This class serves as the central controller for the x86 emulator,
 * managing all hardware components, initialization, execution, and shutdown.
 */
class Emulator : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Construct a new Emulator instance
     */
    Emulator();
    
    /**
     * @brief Destroy the Emulator instance
     */
    ~Emulator();
    
    /**
     * @brief Initialize the emulator and all its components
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();
    
    /**
     * @brief Start the emulation
     * 
     * @return true if the emulation was started successfully
     * @return false if the emulation could not be started
     */
    bool start();
    
    /**
     * @brief Stop the emulation
     */
    void stop();
    
    /**
     * @brief Pause the emulation
     */
    void pause();
    
    /**
     * @brief Resume the emulation if paused
     */
    void resume();
    
    /**
     * @brief Reset the emulated system
     */
    void reset();
    
    /**
     * @brief Hard reset (power cycle) the emulated system
     */
    void hardReset();
    
    /**
     * @brief Execute a single frame of emulation
     * 
     * @return Number of cycles executed
     */
    int runFrame();
    
    /**
     * @brief Get the framebuffer of the emulated display
     * 
     * @param image Reference to QImage that will be filled with framebuffer data
     * @return true if the framebuffer was successfully retrieved
     * @return false if the framebuffer could not be retrieved
     */
    bool getFrameBuffer(QImage& image);
    
    /**
     * @brief Send keyboard input to the emulator
     * 
     * @param key Key code
     * @param modifiers Key modifiers
     */
    void keyPressed(int key, int modifiers);
    
    /**
     * @brief Send keyboard release to the emulator
     * 
     * @param key Key code
     * @param modifiers Key modifiers
     */
    void keyReleased(int key, int modifiers);
    
    /**
     * @brief Send mouse button press to the emulator
     * 
     * @param button Mouse button
     */
    void mouseButtonPressed(int button);
    
    /**
     * @brief Send mouse button release to the emulator
     * 
     * @param button Mouse button
     */
    void mouseButtonReleased(int button);
    
    /**
     * @brief Send mouse movement to the emulator
     * 
     * @param xRel Relative X movement
     * @param yRel Relative Y movement
     */
    void mouseMove(int xRel, int yRel);
    
    /**
     * @brief Send mouse wheel movement to the emulator
     * 
     * @param delta Wheel delta
     */
    void mouseWheelMove(int delta);
    
    /**
     * @brief Send Ctrl+Alt+Del keypress to the emulator
     */
    void sendCtrlAltDel();
    
    /**
     * @brief Mount a floppy disk image
     * 
     * @param drive Drive number (0=A:, 1=B:)
     * @param path Path to the disk image file
     * @return true if mounted successfully
     * @return false if mount failed
     */
    bool mountFloppyDisk(int drive, const std::string& path);
    
    /**
     * @brief Eject a floppy disk
     * 
     * @param drive Drive number (0=A:, 1=B:)
     */
    void ejectFloppyDisk(int drive);
    
    /**
     * @brief Mount a CD-ROM image
     * 
     * @param path Path to the CD-ROM image file
     * @return true if mounted successfully
     * @return false if mount failed
     */
    bool mountCDROM(const std::string& path);
    
    /**
     * @brief Eject the CD-ROM
     */
    void ejectCDROM();
    
    /**
     * @brief Create a new hard disk image
     * 
     * @param path Path to create the disk image
     * @param sizeInMB Size in megabytes
     * @param type Disk type (e.g., "ide", "scsi")
     * @return true if the disk was created successfully
     * @return false if the disk could not be created
     */
    bool createHardDisk(const std::string& path, int sizeInMB, const std::string& type);
    
    /**
     * @brief Mount a hard disk image
     * 
     * @param path Path to the hard disk image
     * @param type Disk type (e.g., "ide", "scsi")
     * @param channel Channel/index to mount the disk on
     * @return true if the disk was mounted successfully
     * @return false if the disk could not be mounted
     */
    bool mountHardDisk(const std::string& path, const std::string& type, int channel);
    
    /**
     * @brief Unmount a hard disk
     * 
     * @param type Disk type (e.g., "ide", "scsi")
     * @param channel Channel/index to unmount
     */
    void unmountHardDisk(const std::string& type, int channel);
    
    /**
     * @brief Get the display width
     * 
     * @return Display width in pixels
     */
    int getDisplayWidth() const;
    
    /**
     * @brief Get the display height
     * 
     * @return Display height in pixels
     */
    int getDisplayHeight() const;
    
    /**
     * @brief Check if a floppy drive is showing activity
     * 
     * @param drive Drive number (0=A:, 1=B:)
     * @return true if there is activity
     * @return false if there is no activity
     */
    bool getFloppyActivity(int drive) const;
    
    /**
     * @brief Check if the CD-ROM drive is showing activity
     * 
     * @return true if there is activity
     * @return false if there is no activity
     */
    bool getCDROMActivity() const;
    
    /**
     * @brief Check if a hard disk is showing activity
     * 
     * @param index Disk index
     * @return true if there is activity
     * @return false if there is no activity
     */
    bool getHardDiskActivity(int index) const;
    
    /**
     * @brief Check if there is network activity
     * 
     * @return true if there is activity
     * @return false if there is no activity
     */
    bool getNetworkActivity() const;
    
    /**
     * @brief Get the ConfigManager instance
     * 
     * @return Pointer to the ConfigManager
     */
    ConfigManager* getConfigManager() const { return m_configManager.get(); }

    /**
     * @brief Get the CPU backend type
     * 
     * @return CPU backend type as a string ("MAME", "86Box", or "Custom")
     */
    std::string getCpuBackendType() const;

    /**
     * @brief Register a callback for boot state changes
     * 
     * @param callback The function to call when boot state changes
     */
    void registerBootStateCallback(std::function<void(uint8_t)> callback);

signals:
    /**
     * @brief Signal emitted when the emulator state changes
     */
    void emulationStateChanged(bool running, bool paused);
    
    /**
     * @brief Signal emitted when a floppy disk is mounted or unmounted
     */
    void floppyMountChanged(int drive, bool mounted);
    
    /**
     * @brief Signal emitted when the CD-ROM is mounted or unmounted
     */
    void cdromMountChanged(bool mounted);
    
    /**
     * @brief Signal emitted when a hard disk is mounted or unmounted
     */
    void hardDiskMountChanged(int index, bool mounted);

private:
    /**
     * @brief Initialize the CPU
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initializeCPU();
    
    /**
     * @brief Initialize the memory subsystem
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initializeMemory();
    
    /**
     * @brief Initialize the I/O subsystem
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initializeIO();
    
    /**
     * @brief Initialize devices
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initializeDevices();
    
    /**
     * @brief Initialize BIOS
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initializeBIOS();
    
    /**
     * @brief Load configuration
     */
    void loadConfiguration();
    
    /**
     * @brief Update emulation state and notify listeners
     */
    void updateEmulationState();
    
    /**
     * @brief Handle CPU boot state changes
     * 
     * @param state Boot state code
     */
    void onBootStateChange(uint8_t state);

private:
    // Core components
    std::unique_ptr<ConfigManager> m_configManager;
    std::unique_ptr<x86emu::X86CPU> m_cpu;
    std::unique_ptr<MemoryManager> m_memory;
    std::unique_ptr<IOManager> m_io;
    std::unique_ptr<InterruptController> m_intController;
    std::unique_ptr<DeviceManager> m_deviceManager;
    std::unique_ptr<TimerManager> m_timerManager;
    
    // I/O devices
    std::unique_ptr<GraphicsAdapter> m_graphicsAdapter;
    std::unique_ptr<SoundManager> m_soundManager;
    std::vector<std::unique_ptr<FloppyDrive>> m_floppyDrives;
    std::unique_ptr<CDROMDrive> m_cdromDrive;
    std::vector<std::unique_ptr<HardDisk>> m_hardDisks;
    std::unique_ptr<NetworkAdapter> m_networkAdapter;
    
    // Emulation state
    std::atomic<bool> m_initialized;
    std::atomic<bool> m_running;
    std::atomic<bool> m_paused;
    std::mutex m_emulationMutex;
    
    // Boot state callback
    std::function<void(uint8_t)> m_bootStateCallback;
    
    // Performance tracking
    int m_cyclesPerSecond;
    int m_framesPerSecond;
    
    // Logger
    std::shared_ptr<Logger> m_logger;
};

#endif // X86EMULATOR_EMULATOR_H