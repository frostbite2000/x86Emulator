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

#include "emulator.h"
#include "config_manager.h"
#include "memory_manager.h"
#include "io_manager.h"
#include "interrupt_controller.h"
#include "device_manager.h"
#include "timer_manager.h"
#include "graphics_adapter.h"
#include "sound_manager.h"
#include "floppy_drive.h"
#include "cdrom_drive.h"
#include "hard_disk.h"
#include "network_adapter.h"
#include "logger.h"
#include "devices/cpu/i386/x86_cpu.h"
#include "devices/cpu/i386/x86_cpu_factory.h"

#include <chrono>
#include <thread>
#include <stdexcept>

// Default frame rate (60 Hz)
constexpr int DEFAULT_FRAME_RATE = 60;

// CPU cycles per frame (4.77 MHz CPU at 60 Hz = ~79500 cycles/frame)
constexpr int DEFAULT_CYCLES_PER_FRAME = 79500;

Emulator::Emulator()
    : m_initialized(false), 
      m_running(false), 
      m_paused(false),
      m_cyclesPerSecond(4770000),  // 4.77 MHz
      m_framesPerSecond(DEFAULT_FRAME_RATE)
{
    // Initialize logger
    m_logger = std::make_shared<Logger>("emulator.log");
    
    // Create configuration manager
    m_configManager = std::make_unique<ConfigManager>();
    
    // Load configuration
    loadConfiguration();
    
    // Initialize floppy drives (typically 2)
    m_floppyDrives.resize(2);
    
    // Initialize hard disks (reserve space for 4)
    m_hardDisks.reserve(4);
}

Emulator::~Emulator()
{
    // Ensure emulation is stopped
    stop();
    
    // Clean up devices in reverse order of initialization
    m_networkAdapter.reset();
    m_hardDisks.clear();
    m_cdromDrive.reset();
    m_floppyDrives.clear();
    m_soundManager.reset();
    m_graphicsAdapter.reset();
    m_timerManager.reset();
    m_deviceManager.reset();
    m_intController.reset();
    m_io.reset();
    m_memory.reset();
    m_cpu.reset();
    
    // Save configuration
    if (m_configManager) {
        m_configManager->saveConfiguration();
    }
    m_configManager.reset();
}

bool Emulator::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    m_logger->info("Initializing emulator...");
    
    try {
        // Initialize memory subsystem
        if (!initializeMemory()) {
            m_logger->error("Memory initialization failed");
            return false;
        }
        
        // Initialize I/O subsystem
        if (!initializeIO()) {
            m_logger->error("I/O initialization failed");
            return false;
        }
        
        // Initialize interrupt controller
        m_intController = std::make_unique<InterruptController>();
        if (!m_intController->initialize()) {
            m_logger->error("Interrupt controller initialization failed");
            return false;
        }
        
        // Initialize CPU
        if (!initializeCPU()) {
            m_logger->error("CPU initialization failed");
            return false;
        }
        
        // Initialize devices
        if (!initializeDevices()) {
            m_logger->error("Device initialization failed");
            return false;
        }
        
        // Initialize BIOS
        if (!initializeBIOS()) {
            m_logger->error("BIOS initialization failed");
            return false;
        }
        
        m_initialized = true;
        m_logger->info("Emulator initialized successfully");
        return true;
        
    } catch (const std::exception& ex) {
        m_logger->error("Exception during initialization: %s", ex.what());
        return false;
    }
}

bool Emulator::initializeCPU()
{
    m_logger->info("Initializing CPU...");
    
    try {
        // Get CPU configuration
        std::string cpuModel = m_configManager->getString("cpu", "type", "i386");
        
        // Get CPU backend from configuration
        x86emu::CPUBackendType backendType = x86emu::X86CPUFactory::GetDefaultBackendType();
        
        // Create CPU instance
        m_cpu = std::make_unique<x86emu::X86CPU>(cpuModel, backendType);
        
        // Set up memory and I/O handlers
        // This would typically involve setting up callbacks or access methods
        
        // Register boot state callback
        // This would set up a callback to be notified of CPU state changes
        
        // Initialize the CPU
        if (!m_cpu->Initialize()) {
            m_logger->error("CPU initialization failed");
            return false;
        }
        
        m_logger->info("CPU initialized: %s (using %s backend)", 
                      cpuModel.c_str(),
                      getCpuBackendType().c_str());
        
        return true;
        
    } catch (const std::exception& ex) {
        m_logger->error("Exception during CPU initialization: %s", ex.what());
        return false;
    }
}

bool Emulator::initializeMemory()
{
    m_logger->info("Initializing memory subsystem...");
    
    try {
        // Get memory configuration
        int memorySize = m_configManager->getInt("memory", "size", 640);
        
        // Create memory manager
        m_memory = std::make_unique<MemoryManager>();
        
        // Configure memory
        if (!m_memory->initialize(memorySize)) {
            m_logger->error("Memory manager initialization failed");
            return false;
        }
        
        m_logger->info("Memory initialized: %d KB", memorySize);
        return true;
        
    } catch (const std::exception& ex) {
        m_logger->error("Exception during memory initialization: %s", ex.what());
        return false;
    }
}

bool Emulator::initializeIO()
{
    m_logger->info("Initializing I/O subsystem...");
    
    try {
        // Create I/O manager
        m_io = std::make_unique<IOManager>();
        
        // Configure I/O ports
        if (!m_io->initialize()) {
            m_logger->error("I/O manager initialization failed");
            return false;
        }
        
        m_logger->info("I/O subsystem initialized");
        return true;
        
    } catch (const std::exception& ex) {
        m_logger->error("Exception during I/O initialization: %s", ex.what());
        return false;
    }
}

bool Emulator::initializeDevices()
{
    m_logger->info("Initializing devices...");
    
    try {
        // Initialize device manager
        m_deviceManager = std::make_unique<DeviceManager>();
        if (!m_deviceManager->initialize()) {
            m_logger->error("Device manager initialization failed");
            return false;
        }
        
        // Initialize timer manager
        m_timerManager = std::make_unique<TimerManager>();
        if (!m_timerManager->initialize()) {
            m_logger->error("Timer manager initialization failed");
            return false;
        }
        
        // Initialize graphics adapter
        std::string videoCard = m_configManager->getString("video", "card", "vga");
        m_graphicsAdapter = std::make_unique<GraphicsAdapter>(videoCard);
        if (!m_graphicsAdapter->initialize()) {
            m_logger->error("Graphics adapter initialization failed");
            return false;
        }
        
        // Initialize sound card
        std::string soundCard = m_configManager->getString("sound", "card", "sb16");
        m_soundManager = std::make_unique<SoundManager>(soundCard);
        if (!m_soundManager->initialize()) {
            m_logger->error("Sound manager initialization failed");
            return false;
        }
        
        // Initialize floppy drives
        for (int i = 0; i < 2; i++) {
            std::string driveKey = (i == 0) ? "a_type" : "b_type";
            std::string driveType = m_configManager->getString("floppy", driveKey, (i == 0) ? "3.5\" 1.44MB" : "none");
            
            if (driveType != "none") {
                m_floppyDrives[i] = std::make_unique<FloppyDrive>(driveType, i);
                if (!m_floppyDrives[i]->initialize()) {
                    m_logger->error("Floppy drive %d initialization failed", i);
                    return false;
                }
            }
        }
        
        // Initialize CD-ROM drive
        std::string cdromType = m_configManager->getString("cdrom", "type", "none");
        if (cdromType != "none") {
            m_cdromDrive = std::make_unique<CDROMDrive>(cdromType);
            if (!m_cdromDrive->initialize()) {
                m_logger->error("CD-ROM drive initialization failed");
                return false;
            }
        }
        
        // Initialize hard disks from configuration
        int numHardDisks = m_configManager->getInt("harddisk", "count", 0);
        for (int i = 0; i < numHardDisks; i++) {
            std::string idxStr = std::to_string(i);
            std::string diskType = m_configManager->getString("harddisk", "type" + idxStr, "ide");
            std::string diskPath = m_configManager->getString("harddisk", "path" + idxStr, "");
            
            if (!diskPath.empty()) {
                auto disk = std::make_unique<HardDisk>(diskType, i);
                if (disk->initialize() && disk->mount(diskPath)) {
                    m_hardDisks.push_back(std::move(disk));
                } else {
                    m_logger->error("Hard disk %d initialization failed", i);
                    // Continue with other disks
                }
            }
        }
        
        // Initialize network adapter
        std::string networkType = m_configManager->getString("network", "type", "none");
        if (networkType != "none") {
            std::string networkCard = m_configManager->getString("network", "card", "ne2000");
            m_networkAdapter = std::make_unique<NetworkAdapter>(networkCard);
            if (!m_networkAdapter->initialize()) {
                m_logger->error("Network adapter initialization failed");
                // Not critical, continue
            }
        }
        
        m_logger->info("Devices initialized");
        return true;
        
    } catch (const std::exception& ex) {
        m_logger->error("Exception during device initialization: %s", ex.what());
        return false;
    }
}

bool Emulator::initializeBIOS()
{
    m_logger->info("Initializing BIOS...");
    
    try {
        // Load BIOS configuration
        std::string biosPath = m_configManager->getString("bios", "path", "bios.bin");
        
        // Load BIOS into memory
        if (!m_memory->loadBIOS(biosPath)) {
            m_logger->error("Failed to load BIOS from %s", biosPath.c_str());
            return false;
        }
        
        // Reset CPU to start execution from BIOS entry point
        m_cpu->Reset();
        
        m_logger->info("BIOS initialized");
        return true;
        
    } catch (const std::exception& ex) {
        m_logger->error("Exception during BIOS initialization: %s", ex.what());
        return false;
    }
}

void Emulator::loadConfiguration()
{
    m_logger->info("Loading configuration...");
    
    try {
        // Load configuration from file
        if (!m_configManager->loadConfiguration()) {
            m_logger->warn("Failed to load configuration, using defaults");
        }
        
        // Get CPU speed
        m_cyclesPerSecond = static_cast<int>(m_configManager->getDouble("cpu", "frequency", 4.77) * 1000000);
        
        // Get frame rate
        m_framesPerSecond = m_configManager->getInt("timing", "framerate", DEFAULT_FRAME_RATE);
        
    } catch (const std::exception& ex) {
        m_logger->error("Exception during configuration loading: %s", ex.what());
    }
}

bool Emulator::start()
{
    std::lock_guard<std::mutex> lock(m_emulationMutex);
    
    if (m_running) {
        return true;  // Already running
    }
    
    if (!m_initialized && !initialize()) {
        m_logger->error("Cannot start emulation, initialization failed");
        return false;
    }
    
    m_logger->info("Starting emulation...");
    
    // Reset devices to ensure clean state
    reset();
    
    // Start emulation
    m_running = true;
    m_paused = false;
    
    // Notify listeners
    updateEmulationState();
    
    m_logger->info("Emulation started");
    return true;
}

void Emulator::stop()
{
    std::lock_guard<std::mutex> lock(m_emulationMutex);
    
    if (!m_running) {
        return;  // Not running
    }
    
    m_logger->info("Stopping emulation...");
    
    // Stop CPU
    if (m_cpu) {
        m_cpu->Stop();
    }
    
    // Update state
    m_running = false;
    m_paused = false;
    
    // Notify listeners
    updateEmulationState();
    
    m_logger->info("Emulation stopped");
}

void Emulator::pause()
{
    std::lock_guard<std::mutex> lock(m_emulationMutex);
    
    if (!m_running || m_paused) {
        return;  // Not running or already paused
    }
    
    m_logger->info("Pausing emulation...");
    
    // Pause CPU
    if (m_cpu) {
        m_cpu->Pause();
    }
    
    // Pause devices
    if (m_timerManager) {
        m_timerManager->pause();
    }
    
    // Update state
    m_paused = true;
    
    // Notify listeners
    updateEmulationState();
    
    m_logger->info("Emulation paused");
}

void Emulator::resume()
{
    std::lock_guard<std::mutex> lock(m_emulationMutex);
    
    if (!m_running || !m_paused) {
        return;  // Not running or not paused
    }
    
    m_logger->info("Resuming emulation...");
    
    // Resume CPU
    if (m_cpu) {
        m_cpu->Resume();
    }
    
    // Resume devices
    if (m_timerManager) {
        m_timerManager->resume();
    }
    
    // Update state
    m_paused = false;
    
    // Notify listeners
    updateEmulationState();
    
    m_logger->info("Emulation resumed");
}

void Emulator::reset()
{
    std::lock_guard<std::mutex> lock(m_emulationMutex);
    
    m_logger->info("Resetting system...");
    
    // Reset CPU
    if (m_cpu) {
        m_cpu->Reset();
    }
    
    // Reset devices
    if (m_timerManager) {
        m_timerManager->reset();
    }
    
    if (m_intController) {
        m_intController->reset();
    }
    
    if (m_graphicsAdapter) {
        m_graphicsAdapter->reset();
    }
    
    if (m_soundManager) {
        m_soundManager->reset();
    }
    
    // Reset other devices...
    
    m_logger->info("System reset complete");
}

void Emulator::hardReset()
{
    std::lock_guard<std::mutex> lock(m_emulationMutex);
    
    m_logger->info("Performing hard reset (power cycle)...");
    
    // Stop emulation temporarily
    bool wasRunning = m_running;
    bool wasPaused = m_paused;
    
    m_running = false;
    m_paused = false;
    
    // Reinitialize CPU and devices
    initializeCPU();
    initializeDevices();
    initializeBIOS();
    
    // Restore emulation state
    m_running = wasRunning;
    m_paused = wasPaused;
    
    // Notify listeners
    updateEmulationState();
    
    m_logger->info("Hard reset (power cycle) complete");
}

int Emulator::runFrame()
{
    if (!m_running || m_paused) {
        return 0;
    }
    
    try {
        // Calculate cycles for this frame
        int cyclesPerFrame = m_cyclesPerSecond / m_framesPerSecond;
        
        // Run CPU for the calculated number of cycles
        int executedCycles = m_cpu->Execute(cyclesPerFrame);
        
        // Update timers
        if (m_timerManager) {
            m_timerManager->update(executedCycles);
        }
        
        // Update devices
        if (m_deviceManager) {
            m_deviceManager->update(executedCycles);
        }
        
        return executedCycles;
        
    } catch (const std::exception& ex) {
        m_logger->error("Exception during frame execution: %s", ex.what());
        return 0;
    }
}

bool Emulator::getFrameBuffer(QImage& image)
{
    if (!m_graphicsAdapter || !m_running) {
        return false;
    }
    
    try {
        return m_graphicsAdapter->getFrameBuffer(image);
    } catch (const std::exception& ex) {
        m_logger->error("Exception while getting framebuffer: %s", ex.what());
        return false;
    }
}

void Emulator::keyPressed(int key, int modifiers)
{
    if (!m_running || m_paused) {
        return;
    }
    
    try {
        // Handle key press by forwarding to device manager
        if (m_deviceManager) {
            m_deviceManager->keyPressed(key, modifiers);
        }
    } catch (const std::exception& ex) {
        m_logger->error("Exception during key press handling: %s", ex.what());
    }
}

void Emulator::keyReleased(int key, int modifiers)
{
    if (!m_running || m_paused) {
        return;
    }
    
    try {
        // Handle key release by forwarding to device manager
        if (m_deviceManager) {
            m_deviceManager->keyReleased(key, modifiers);
        }
    } catch (const std::exception& ex) {
        m_logger->error("Exception during key release handling: %s", ex.what());
    }
}

void Emulator::mouseButtonPressed(int button)
{
    if (!m_running || m_paused) {
        return;
    }
    
    try {
        // Handle mouse button press by forwarding to device manager
        if (m_deviceManager) {
            m_deviceManager->mouseButtonPressed(button);
        }
    } catch (const std::exception& ex) {
        m_logger->error("Exception during mouse button press handling: %s", ex.what());
    }
}

void Emulator::mouseButtonReleased(int button)
{
    if (!m_running || m_paused) {
        return;
    }
    
    try {
        // Handle mouse button release by forwarding to device manager
        if (m_deviceManager) {
            m_deviceManager->mouseButtonReleased(button);
        }
    } catch (const std::exception& ex) {
        m_logger->error("Exception during mouse button release handling: %s", ex.what());
    }
}

void Emulator::mouseMove(int xRel, int yRel)
{
    if (!m_running || m_paused) {
        return;
    }
    
    try {
        // Handle mouse movement by forwarding to device manager
        if (m_deviceManager) {
            m_deviceManager->mouseMove(xRel, yRel);
        }
    } catch (const std::exception& ex) {
        m_logger->error("Exception during mouse movement handling: %s", ex.what());
    }
}

void Emulator::mouseWheelMove(int delta)
{
    if (!m_running || m_paused) {
        return;
    }
    
    try {
        // Handle mouse wheel movement by forwarding to device manager
        if (m_deviceManager) {
            m_deviceManager->mouseWheelMove(delta);
        }
    } catch (const std::exception& ex) {
        m_logger->error("Exception during mouse wheel movement handling: %s", ex.what());
    }
}

void Emulator::sendCtrlAltDel()
{
    if (!m_running) {
        return;
    }
    
    try {
        // Simulate Ctrl+Alt+Del key sequence
        if (m_deviceManager) {
            m_deviceManager->sendCtrlAltDel();
        }
    } catch (const std::exception& ex) {
        m_logger->error("Exception during Ctrl+Alt+Del simulation: %s", ex.what());
    }
}

bool Emulator::mountFloppyDisk(int drive, const std::string& path)
{
    if (drive < 0 || drive >= static_cast<int>(m_floppyDrives.size()) || !m_floppyDrives[drive]) {
        m_logger->error("Invalid floppy drive: %d", drive);
        return false;
    }
    
    try {
        bool success = m_floppyDrives[drive]->mount(path);
        if (success) {
            m_logger->info("Mounted floppy disk in drive %c: %s", 'A' + drive, path.c_str());
            emit floppyMountChanged(drive, true);
        } else {
            m_logger->error("Failed to mount floppy disk in drive %c: %s", 'A' + drive, path.c_str());
        }
        return success;
    } catch (const std::exception& ex) {
        m_logger->error("Exception during floppy disk mount: %s", ex.what());
        return false;
    }
}

void Emulator::ejectFloppyDisk(int drive)
{
    if (drive < 0 || drive >= static_cast<int>(m_floppyDrives.size()) || !m_floppyDrives[drive]) {
        m_logger->error("Invalid floppy drive: %d", drive);
        return;
    }
    
    try {
        m_floppyDrives[drive]->eject();
        m_logger->info("Ejected floppy disk from drive %c", 'A' + drive);
        emit floppyMountChanged(drive, false);
    } catch (const std::exception& ex) {
        m_logger->error("Exception during floppy disk ejection: %s", ex.what());
    }
}

bool Emulator::mountCDROM(const std::string& path)
{
    if (!m_cdromDrive) {
        m_logger->error("No CD-ROM drive available");
        return false;
    }
    
    try {
        bool success = m_cdromDrive->mount(path);
        if (success) {
            m_logger->info("Mounted CD-ROM: %s", path.c_str());
            emit cdromMountChanged(true);
        } else {
            m_logger->error("Failed to mount CD-ROM: %s", path.c_str());
        }
        return success;
    } catch (const std::exception& ex) {
        m_logger->error("Exception during CD-ROM mount: %s", ex.what());
        return false;
    }
}

void Emulator::ejectCDROM()
{
    if (!m_cdromDrive) {
        m_logger->error("No CD-ROM drive available");
        return;
    }
    
    try {
        m_cdromDrive->eject();
        m_logger->info("Ejected CD-ROM");
        emit cdromMountChanged(false);
    } catch (const std::exception& ex) {
        m_logger->error("Exception during CD-ROM ejection: %s", ex.what());
    }
}

bool Emulator::createHardDisk(const std::string& path, int sizeInMB, const std::string& type)
{
    try {
        // Create a temporary hard disk object
        HardDisk tempDisk(type, -1);
        
        // Create the disk image
        bool success = tempDisk.create(path, sizeInMB);
        
        if (success) {
            m_logger->info("Created hard disk image: %s (%d MB, %s)", 
                          path.c_str(), sizeInMB, type.c_str());
        } else {
            m_logger->error("Failed to create hard disk image: %s", path.c_str());
        }
        
        return success;
    } catch (const std::exception& ex) {
        m_logger->error("Exception during hard disk creation: %s", ex.what());
        return false;
    }
}

bool Emulator::mountHardDisk(const std::string& path, const std::string& type, int channel)
{
    try {
        // Check if the channel is already in use
        for (const auto& disk : m_hardDisks) {
            if (disk->getType() == type && disk->getChannel() == channel) {
                m_logger->error("Hard disk channel already in use: %s %d", type.c_str(), channel);
                return false;
            }
        }
        
        // Create a new hard disk object
        auto disk = std::make_unique<HardDisk>(type, channel);
        
        // Initialize and mount the disk
        if (!disk->initialize() || !disk->mount(path)) {
            m_logger->error("Failed to mount hard disk: %s", path.c_str());
            return false;
        }
        
        // Add the disk to the list
        m_hardDisks.push_back(std::move(disk));
        
        m_logger->info("Mounted hard disk: %s (%s %d)", path.c_str(), type.c_str(), channel);
        emit hardDiskMountChanged(m_hardDisks.size() - 1, true);
        
        return true;
    } catch (const std::exception& ex) {
        m_logger->error("Exception during hard disk mount: %s", ex.what());
        return false;
    }
}

void Emulator::unmountHardDisk(const std::string& type, int channel)
{
    try {
        // Find the disk to unmount
        for (auto it = m_hardDisks.begin(); it != m_hardDisks.end(); ++it) {
            if ((*it)->getType() == type && (*it)->getChannel() == channel) {
                // Remember the index
                int index = static_cast<int>(std::distance(m_hardDisks.begin(), it));
                
                // Unmount and remove the disk
                (*it)->unmount();
                m_hardDisks.erase(it);
                
                m_logger->info("Unmounted hard disk: %s %d", type.c_str(), channel);
                emit hardDiskMountChanged(index, false);
                
                return;
            }
        }
        
        m_logger->error("No hard disk found with type %s and channel %d", type.c_str(), channel);
    } catch (const std::exception& ex) {
        m_logger->error("Exception during hard disk unmount: %s", ex.what());
    }
}

int Emulator::getDisplayWidth() const
{
    if (m_graphicsAdapter) {
        return m_graphicsAdapter->getDisplayWidth();
    }
    return 640;  // Default width
}

int Emulator::getDisplayHeight() const
{
    if (m_graphicsAdapter) {
        return m_graphicsAdapter->getDisplayHeight();
    }
    return 480;  // Default height
}

bool Emulator::getFloppyActivity(int drive) const
{
    if (drive >= 0 && drive < static_cast<int>(m_floppyDrives.size()) && m_floppyDrives[drive]) {
        return m_floppyDrives[drive]->isActive();
    }
    return false;
}

bool Emulator::getCDROMActivity() const
{
    if (m_cdromDrive) {
        return m_cdromDrive->isActive();
    }
    return false;
}

bool Emulator::getHardDiskActivity(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_hardDisks.size())) {
        return m_hardDisks[index]->isActive();
    }
    return false;
}

bool Emulator::getNetworkActivity() const
{
    if (m_networkAdapter) {
        return m_networkAdapter->isActive();
    }
    return false;
}

std::string Emulator::getCpuBackendType() const
{
    if (!m_cpu) {
        return "Unknown";
    }
    
    x86emu::CPUBackendType type = m_cpu->GetBackendType();
    switch (type) {
        case x86emu::CPUBackendType::MAME:
            return "MAME";
        case x86emu::CPUBackendType::BOX86:
            return "86Box";
        case x86emu::CPUBackendType::CUSTOM:
            return "Custom";
        default:
            return "Unknown";
    }
}

void Emulator::registerBootStateCallback(std::function<void(uint8_t)> callback)
{
    m_bootStateCallback = callback;
}

void Emulator::updateEmulationState()
{
    // Emit signal to notify listeners of state change
    emit emulationStateChanged(m_running, m_paused);
}

void Emulator::onBootStateChange(uint8_t state)
{
    // Log the boot state change
    m_logger->info("Boot state change: 0x%02X", state);
    
    // Call the registered callback if available
    if (m_bootStateCallback) {
        m_bootStateCallback(state);
    }
}