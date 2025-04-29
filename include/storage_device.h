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

#ifndef X86EMULATOR_STORAGE_DEVICE_H
#define X86EMULATOR_STORAGE_DEVICE_H

#include "device.h"
#include <cstdint>
#include <string>
#include <mutex>
#include <memory>
#include <vector>
#include <fstream>

/**
 * @brief Base class for storage devices
 */
class StorageDevice : public Device {
public:
    /**
     * @brief Storage device types
     */
    enum class DeviceType {
        FLOPPY,
        CDROM,
        HARDDISK
    };
    
    /**
     * @brief Construct a new Storage Device
     * 
     * @param name Device name
     * @param type Device type
     */
    StorageDevice(const std::string& name, DeviceType type);
    
    /**
     * @brief Destroy the Storage Device
     */
    ~StorageDevice() override;
    
    /**
     * @brief Initialize the storage device
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize() override = 0;
    
    /**
     * @brief Update the storage device
     * 
     * @param cycles Number of CPU cycles elapsed
     */
    void update(int cycles) override = 0;
    
    /**
     * @brief Reset the storage device
     */
    void reset() override = 0;
    
    /**
     * @brief Mount a disk image
     * 
     * @param path Path to the disk image
     * @return true if mounting was successful
     * @return false if mounting failed
     */
    virtual bool mount(const std::string& path) = 0;
    
    /**
     * @brief Eject the disk
     */
    virtual void eject() = 0;
    
    /**
     * @brief Check if a disk is mounted
     * 
     * @return true if a disk is mounted
     * @return false if no disk is mounted
     */
    virtual bool isMounted() const = 0;
    
    /**
     * @brief Check if the device is active (reading/writing)
     * 
     * @return true if the device is active
     * @return false if the device is not active
     */
    bool isActive() const override;
    
    /**
     * @brief Get the device type
     * 
     * @return Device type
     */
    DeviceType getDeviceType() const;
    
    /**
     * @brief Get the mounted image path
     * 
     * @return Path to mounted image, or empty string if no image is mounted
     */
    std::string getMountedImagePath() const;
    
    /**
     * @brief Get the last error message
     * 
     * @return Error message
     */
    std::string getLastError() const;
    
protected:
    DeviceType m_deviceType;
    std::string m_mountedImagePath;
    std::string m_lastError;
    bool m_active;
    mutable std::mutex m_mutex;
};

#endif // X86EMULATOR_STORAGE_DEVICE_H