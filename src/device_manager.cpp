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

#include "device_manager.h"
#include "device.h"
#include "logger.h"

DeviceManager::DeviceManager()
{
}

DeviceManager::~DeviceManager()
{
    // Clear all devices
    m_devices.clear();
    m_deviceMap.clear();
}

bool DeviceManager::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Clear device lists
        m_devices.clear();
        m_deviceMap.clear();
        
        Logger::GetInstance()->info("Device manager initialized");
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Device manager initialization failed: %s", ex.what());
        return false;
    }
}

bool DeviceManager::registerDevice(std::shared_ptr<Device> device)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!device) {
        Logger::GetInstance()->error("Cannot register null device");
        return false;
    }
    
    // Get device name
    std::string name = device->getName();
    
    // Check if device with this name already exists
    if (m_deviceMap.find(name) != m_deviceMap.end()) {
        Logger::GetInstance()->error("Device with name '%s' already registered", name.c_str());
        return false;
    }
    
    // Register device
    m_devices.push_back(device);
    m_deviceMap[name] = device;
    
    Logger::GetInstance()->info("Registered device: %s", name.c_str());
    return true;
}

bool DeviceManager::unregisterDevice(std::shared_ptr<Device> device)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!device) {
        Logger::GetInstance()->error("Cannot unregister null device");
        return false;
    }
    
    // Get device name
    std::string name = device->getName();
    
    // Remove from device map
    auto mapIt = m_deviceMap.find(name);
    if (mapIt != m_deviceMap.end()) {
        m_deviceMap.erase(mapIt);
    }
    
    // Remove from device vector
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        if (*it == device) {
            m_devices.erase(it);
            
            Logger::GetInstance()->info("Unregistered device: %s", name.c_str());
            return true;
        }
    }
    
    Logger::GetInstance()->warn("Device '%s' not found for unregistration", name.c_str());
    return false;
}

bool DeviceManager::unregisterDevice(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find device in map
    auto mapIt = m_deviceMap.find(name);
    if (mapIt == m_deviceMap.end()) {
        Logger::GetInstance()->warn("Device '%s' not found for unregistration", name.c_str());
        return false;
    }
    
    // Get device pointer
    auto device = mapIt->second;
    
    // Remove from device map
    m_deviceMap.erase(mapIt);
    
    // Remove from device vector
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        if (*it == device) {
            m_devices.erase(it);
            break;
        }
    }
    
    Logger::GetInstance()->info("Unregistered device: %s", name.c_str());
    return true;
}

std::shared_ptr<Device> DeviceManager::getDevice(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find device in map
    auto it = m_deviceMap.find(name);
    if (it != m_deviceMap.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<Device>> DeviceManager::getDevices() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_devices;
}

void DeviceManager::update(int cycles)
{
    // No lock to avoid deadlock with device methods that might access the device manager
    
    // Update all devices
    for (auto& device : m_devices) {
        device->update(cycles);
    }
}

void DeviceManager::keyPressed(int key, int modifiers)
{
    // No lock to avoid deadlock with device methods that might access the device manager
    
    // Forward to all keyboard input devices
    for (auto& device : m_devices) {
        if (device->isKeyboardDevice()) {
            device->keyPressed(key, modifiers);
        }
    }
}

void DeviceManager::keyReleased(int key, int modifiers)
{
    // No lock to avoid deadlock with device methods that might access the device manager
    
    // Forward to all keyboard input devices
    for (auto& device : m_devices) {
        if (device->isKeyboardDevice()) {
            device->keyReleased(key, modifiers);
        }
    }
}

void DeviceManager::mouseButtonPressed(int button)
{
    // No lock to avoid deadlock with device methods that might access the device manager
    
    // Forward to all mouse input devices
    for (auto& device : m_devices) {
        if (device->isMouseDevice()) {
            device->mouseButtonPressed(button);
        }
    }
}

void DeviceManager::mouseButtonReleased(int button)
{
    // No lock to avoid deadlock with device methods that might access the device manager
    
    // Forward to all mouse input devices
    for (auto& device : m_devices) {
        if (device->isMouseDevice()) {
            device->mouseButtonReleased(button);
        }
    }
}

void DeviceManager::mouseMove(int xRel, int yRel)
{
    // No lock to avoid deadlock with device methods that might access the device manager
    
    // Forward to all mouse input devices
    for (auto& device : m_devices) {
        if (device->isMouseDevice()) {
            device->mouseMove(xRel, yRel);
        }
    }
}

void DeviceManager::mouseWheelMove(int delta)
{
    // No lock to avoid deadlock with device methods that might access the device manager
    
    // Forward to all mouse input devices
    for (auto& device : m_devices) {
        if (device->isMouseDevice()) {
            device->mouseWheelMove(delta);
        }
    }
}

void DeviceManager::sendCtrlAltDel()
{
    // No lock to avoid deadlock with device methods that might access the device manager
    
    // Forward to all keyboard input devices
    for (auto& device : m_devices) {
        if (device->isKeyboardDevice()) {
            device->sendCtrlAltDel();
        }
    }
}

void DeviceManager::reset()
{
    // No lock to avoid deadlock with device methods that might access the device manager
    
    // Reset all devices
    for (auto& device : m_devices) {
        device->reset();
    }
    
    Logger::GetInstance()->info("All devices reset");
}