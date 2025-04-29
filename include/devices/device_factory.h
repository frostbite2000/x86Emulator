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

#ifndef DEVICE_FACTORY_H
#define DEVICE_FACTORY_H

#include "device.h"
#include "device_config.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

/**
 * @brief Type definition for device factory functions.
 * 
 * A factory function that creates and returns a device instance based on configuration.
 */
using DeviceFactoryFunction = std::function<std::shared_ptr<Device>(const DeviceConfig&)>;

/**
 * @brief Singleton factory class for creating device instances.
 * 
 * This factory handles creation of various device types like CPUs, chipsets,
 * video cards, sound cards, etc. based on a unified configuration interface.
 */
class DeviceFactory {
public:
    /**
     * @brief Get the singleton instance of DeviceFactory.
     * 
     * @return DeviceFactory& Reference to the singleton instance.
     */
    static DeviceFactory& getInstance();
    
    /**
     * @brief Create a device instance of the specified type.
     * 
     * @param type The type identifier of the device to create.
     * @param config Configuration parameters for the device.
     * @return std::shared_ptr<Device> The created device instance.
     * @throws std::runtime_error if the device type is unknown.
     */
    std::shared_ptr<Device> createDevice(const std::string& type, const DeviceConfig& config);
    
    /**
     * @brief Register a new device type with its factory function.
     * 
     * @param type The type identifier for the device.
     * @param factoryFunc The factory function that creates instances of this device.
     */
    void registerDeviceType(const std::string& type, DeviceFactoryFunction factoryFunc);
    
    /**
     * @brief Get a list of all available device types.
     * 
     * @return std::vector<std::string> List of device type identifiers.
     */
    std::vector<std::string> getAvailableDeviceTypes() const;
    
    /**
     * @brief Get a list of devices in a specific category.
     * 
     * @param category The category to filter by (e.g., "cpu", "video", "sound").
     * @return std::vector<std::string> List of device type identifiers in the category.
     */
    std::vector<std::string> getAvailableDevicesByCategory(const std::string& category) const;
    
    /**
     * @brief Get a list of all device categories.
     * 
     * @return std::vector<std::string> List of category names.
     */
    std::vector<std::string> getAllCategories() const;
    
    /**
     * @brief Register a device type with its category.
     * 
     * @param type The device type identifier.
     * @param category The category the device belongs to.
     */
    void registerDeviceCategory(const std::string& type, const std::string& category);

private:
    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    DeviceFactory();
    
    /**
     * @brief Private destructor to enforce singleton pattern.
     */
    ~DeviceFactory();
    
    // Delete copy and move constructors and assignment operators
    DeviceFactory(const DeviceFactory&) = delete;
    DeviceFactory& operator=(const DeviceFactory&) = delete;
    DeviceFactory(DeviceFactory&&) = delete;
    DeviceFactory& operator=(DeviceFactory&&) = delete;
    
    /**
     * @brief Register all available device types.
     */
    void RegisterAllDevices();
    
    /**
     * @brief Register CPU device types.
     */
    void RegisterCPUDevices();
    
    /**
     * @brief Register chipset device types.
     */
    void RegisterChipsetDevices();
    
    /**
     * @brief Register video device types.
     */
    void RegisterVideoDevices();
    
    /**
     * @brief Register sound device types.
     */
    void RegisterSoundDevices();
    
    /**
     * @brief Register storage device types.
     */
    void RegisterStorageDevices();
    
    /**
     * @brief Register input device types.
     */
    void RegisterInputDevices();
    
    /**
     * @brief Register network device types.
     */
    void RegisterNetworkDevices();
    
    /**
     * @brief Register bus controller device types.
     */
    void RegisterBusDevices();
    
    /**
     * @brief Register peripheral device types.
     */
    void RegisterPeripheralDevices();
    
    /**
     * @brief Map of device type identifiers to their factory functions.
     */
    std::unordered_map<std::string, DeviceFactoryFunction> m_factoryFunctions;
    
    /**
     * @brief Map of device type identifiers to their categories.
     */
    std::unordered_map<std::string, std::string> m_deviceCategories;
};

#endif // DEVICE_FACTORY_H