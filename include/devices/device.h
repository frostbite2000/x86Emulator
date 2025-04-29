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

#ifndef DEVICE_H
#define DEVICE_H

#include "device_config.h"
#include <string>
#include <memory>

/**
 * @brief Base class for all emulated devices.
 * 
 * Provides a common interface for device initialization, configuration,
 * and management in the emulator.
 */
class Device {
public:
    /**
     * @brief Constructor with configuration.
     * 
     * @param config Configuration parameters for the device.
     */
    explicit Device(const DeviceConfig& config) : m_config(config), m_initialized(false) {}
    
    /**
     * @brief Virtual destructor.
     */
    virtual ~Device() = default;
    
    /**
     * @brief Initialize the device.
     * 
     * This method should be called after construction to set up the device.
     * 
     * @return bool True if initialization was successful, false otherwise.
     */
    virtual bool Initialize() = 0;
    
    /**
     * @brief Reset the device to its initial state.
     * 
     * This method should reset the device to its power-on state.
     */
    virtual void Reset() = 0;
    
    /**
     * @brief Check if the device is initialized.
     * 
     * @return bool True if the device is initialized, false otherwise.
     */
    bool IsInitialized() const { return m_initialized; }
    
    /**
     * @brief Get the device name.
     * 
     * @return std::string The name of the device.
     */
    virtual std::string GetName() const = 0;
    
    /**
     * @brief Get the device type.
     * 
     * @return std::string The type of the device (e.g., "cpu", "video", etc.).
     */
    virtual std::string GetType() const = 0;
    
    /**
     * @brief Get the device configuration.
     * 
     * @return const DeviceConfig& Reference to the device configuration.
     */
    const DeviceConfig& GetConfig() const { return m_config; }
    
    /**
     * @brief Update device configuration.
     * 
     * Apply new configuration parameters to the device. This may require
     * re-initialization depending on the parameter changes.
     * 
     * @param config New configuration parameters.
     * @return bool True if configuration was successfully updated.
     */
    virtual bool UpdateConfig(const DeviceConfig& config) {
        m_config = config;
        return true;
    }
    
protected:
    /**
     * @brief Set the initialized state of the device.
     * 
     * @param initialized New initialized state.
     */
    void SetInitialized(bool initialized) { m_initialized = initialized; }
    
    /**
     * @brief The device configuration parameters.
     */
    DeviceConfig m_config;
    
    /**
     * @brief Flag indicating if the device is initialized.
     */
    bool m_initialized;
};

#endif // DEVICE_H