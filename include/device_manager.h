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

#ifndef X86EMULATOR_DEVICE_MANAGER_H
#define X86EMULATOR_DEVICE_MANAGER_H

#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include <functional>
#include <map>

// Forward declarations
class Device;

/**
 * @brief Manager for emulated devices
 * 
 * Handles device registration, initialization, and updates
 */
class DeviceManager {
public:
    /**
     * @brief Construct a new Device Manager
     */
    DeviceManager();
    
    /**
     * @brief Destroy the Device Manager
     */
    ~DeviceManager();
    
    /**
     * @brief Initialize the device manager
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();
    
    /**
     * @brief Register a device
     * 
     * @param device Device instance
     * @return true if registration was successful
     * @return false if registration failed
     */
    bool registerDevice(std::shared_ptr<Device> device);
    
    /**
     * @brief Unregister a device
     * 
     * @param device Device instance
     * @return true if unregistration was successful
     * @return false if unregistration failed
     */
    bool unregisterDevice(std::shared_ptr<Device> device);
    
    /**
     * @brief Unregister a device by name
     * 
     * @param name Device name
     * @return true if unregistration was successful
     * @return false if unregistration failed
     */
    bool unregisterDevice(const std::string& name);
    
    /**
     * @brief Get a device by name
     * 
     * @param name Device name
     * @return std::shared_ptr<Device> Device instance, or nullptr if not found
     */
    std::shared_ptr<Device> getDevice(const std::string& name) const;
    
    /**
     * @brief Get all registered devices
     * 
     * @return std::vector<std::shared_ptr<Device>> List of all devices
     */
    std::vector<std::shared_ptr<Device>> getDevices() const;
    
    /**
     * @brief Update all devices
     * 
     * @param cycles Number of CPU cycles elapsed
     */
    void update(int cycles);
    
    /**
     * @brief Handle key press
     * 
     * @param key Key code
     * @param modifiers Key modifiers
     */
    void keyPressed(int key, int modifiers);
    
    /**
     * @brief Handle key release
     * 
     * @param key Key code
     * @param modifiers Key modifiers
     */
    void keyReleased(int key, int modifiers);
    
    /**
     * @brief Handle mouse button press
     * 
     * @param button Mouse button
     */
    void mouseButtonPressed(int button);
    
    /**
     * @brief Handle mouse button release
     * 
     * @param button Mouse button
     */
    void mouseButtonReleased(int button);
    
    /**
     * @brief Handle mouse movement
     * 
     * @param xRel Relative X movement
     * @param yRel Relative Y movement
     */
    void mouseMove(int xRel, int yRel);
    
    /**
     * @brief Handle mouse wheel movement
     * 
     * @param delta Wheel delta
     */
    void mouseWheelMove(int delta);
    
    /**
     * @brief Send Ctrl+Alt+Del keypress to devices
     */
    void sendCtrlAltDel();
    
    /**
     * @brief Reset all devices
     */
    void reset();

private:
    // Device registry
    std::vector<std::shared_ptr<Device>> m_devices;
    std::map<std::string, std::shared_ptr<Device>> m_deviceMap;
    
    // Mutex for thread safety
    mutable std::mutex m_mutex;
};

#endif // X86EMULATOR_DEVICE_MANAGER_H