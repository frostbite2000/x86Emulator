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

#include "device_factory.h"
#include "devices/cpu/i386/i386.h"
#include "devices/cpu/i486/i486.h"
#include "devices/cpu/pentium/pentium.h"
#include "devices/cpu/pentium_mmx/pentium_mmx.h"
#include "devices/cpu/pentium_pro/pentium_pro.h"
#include "devices/cpu/pentium_ii/pentium_ii.h"
#include "devices/cpu/pentium_iii/pentium_iii.h"
#include "devices/cpu/pentium_4/pentium_4.h"
#include "devices/chipset/i430fx/i430fx.h"
#include "devices/chipset/i430hx/i430hx.h"
#include "devices/chipset/i430vx/i430vx.h"
#include "devices/chipset/i440bx/i440bx.h"
#include "devices/chipset/i440fx/i440fx.h"
#include "devices/chipset/i815/i815.h"
#include "devices/chipset/nforce/nforce.h"
#include "devices/video/vga/vga.h"
#include "devices/video/svga/svga.h"
#include "devices/video/s3virge/s3virge.h"
#include "devices/video/s3trio64/s3trio64.h"
#include "devices/video/geforce3/geforce3.h"
#include "devices/sound/sb16/sb16.h"
#include "devices/sound/es1370/es1370.h"
#include "devices/sound/es1371/es1371.h"
#include "devices/storage/ide/ide.h"
#include "devices/storage/scsi/scsi.h"
#include "devices/storage/floppy/floppy.h"
#include "devices/input/keyboard/keyboard.h"
#include "devices/input/mouse/mouse.h"
#include "devices/input/joystick/joystick.h"
#include "devices/network/ne2000/ne2000.h"
#include "devices/network/rtl8139/rtl8139.h"
#include "devices/bus/pci/pci.h"
#include "devices/bus/isa/isa.h"
#include "devices/bus/usb/usb.h"
#include "devices/peripheral/serial/serial.h"
#include "devices/peripheral/parallel/parallel.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <algorithm>

// DeviceFactory implementation

DeviceFactory::DeviceFactory()
{
    RegisterAllDevices();
}

DeviceFactory::~DeviceFactory()
{
    // Clean up any factory-specific resources
}

DeviceFactory& DeviceFactory::getInstance()
{
    static DeviceFactory instance;
    return instance;
}

std::shared_ptr<Device> DeviceFactory::createDevice(const std::string& type, const DeviceConfig& config)
{
    // Convert type to lowercase for case-insensitive comparison
    std::string lowerType = type;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

    // Find the factory function for this device type
    auto it = m_factoryFunctions.find(lowerType);
    if (it == m_factoryFunctions.end()) {
        throw std::runtime_error("Unknown device type: " + type);
    }

    // Call the factory function to create the device
    return it->second(config);
}

void DeviceFactory::registerDeviceType(const std::string& type, DeviceFactoryFunction factoryFunc)
{
    // Convert type to lowercase for case-insensitive storage
    std::string lowerType = type;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
    
    m_factoryFunctions[lowerType] = factoryFunc;
}

std::vector<std::string> DeviceFactory::getAvailableDeviceTypes() const
{
    std::vector<std::string> types;
    types.reserve(m_factoryFunctions.size());
    
    for (const auto& entry : m_factoryFunctions) {
        types.push_back(entry.first);
    }
    
    return types;
}

std::vector<std::string> DeviceFactory::getAvailableDevicesByCategory(const std::string& category) const
{
    std::vector<std::string> devices;
    
    // Convert category to lowercase for case-insensitive comparison
    std::string lowerCategory = category;
    std::transform(lowerCategory.begin(), lowerCategory.end(), lowerCategory.begin(), ::tolower);
    
    for (const auto& entry : m_deviceCategories) {
        if (entry.second == lowerCategory) {
            devices.push_back(entry.first);
        }
    }
    
    return devices;
}

std::vector<std::string> DeviceFactory::getAllCategories() const
{
    std::vector<std::string> categories;
    std::unordered_map<std::string, bool> uniqueCategories;
    
    for (const auto& entry : m_deviceCategories) {
        if (!uniqueCategories[entry.second]) {
            categories.push_back(entry.second);
            uniqueCategories[entry.second] = true;
        }
    }
    
    return categories;
}

void DeviceFactory::registerDeviceCategory(const std::string& type, const std::string& category)
{
    // Convert both strings to lowercase for case-insensitivity
    std::string lowerType = type;
    std::string lowerCategory = category;
    
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
    std::transform(lowerCategory.begin(), lowerCategory.end(), lowerCategory.begin(), ::tolower);
    
    m_deviceCategories[lowerType] = lowerCategory;
}

void DeviceFactory::RegisterAllDevices()
{
    // Register CPU devices
    RegisterCPUDevices();
    
    // Register chipset devices
    RegisterChipsetDevices();
    
    // Register video devices
    RegisterVideoDevices();
    
    // Register sound devices
    RegisterSoundDevices();
    
    // Register storage devices
    RegisterStorageDevices();
    
    // Register input devices
    RegisterInputDevices();
    
    // Register network devices
    RegisterNetworkDevices();
    
    // Register bus devices
    RegisterBusDevices();
    
    // Register peripheral devices
    RegisterPeripheralDevices();
}

void DeviceFactory::RegisterCPUDevices()
{
    // i386
    registerDeviceType("i386", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<i386CPU>(config);
    });
    registerDeviceCategory("i386", "cpu");
    
    // i486
    registerDeviceType("i486", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<i486CPU>(config);
    });
    registerDeviceCategory("i486", "cpu");
    
    // Pentium
    registerDeviceType("pentium", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<PentiumCPU>(config);
    });
    registerDeviceCategory("pentium", "cpu");
    
    // Pentium MMX
    registerDeviceType("pentium_mmx", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<PentiumMMXCPU>(config);
    });
    registerDeviceCategory("pentium_mmx", "cpu");
    
    // Pentium Pro
    registerDeviceType("pentium_pro", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<PentiumProCPU>(config);
    });
    registerDeviceCategory("pentium_pro", "cpu");
    
    // Pentium II
    registerDeviceType("pentium_ii", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<PentiumIICPU>(config);
    });
    registerDeviceCategory("pentium_ii", "cpu");
    
    // Pentium III
    registerDeviceType("pentium_iii", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<PentiumIIICPU>(config);
    });
    registerDeviceCategory("pentium_iii", "cpu");
    
    // Pentium 4
    registerDeviceType("pentium_4", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<Pentium4CPU>(config);
    });
    registerDeviceCategory("pentium_4", "cpu");
}

void DeviceFactory::RegisterChipsetDevices()
{
    // i430FX
    registerDeviceType("i430fx", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<I430FX>(config);
    });
    registerDeviceCategory("i430fx", "chipset");
    
    // i430HX
    registerDeviceType("i430hx", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<I430HX>(config);
    });
    registerDeviceCategory("i430hx", "chipset");
    
    // i430VX
    registerDeviceType("i430vx", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<I430VX>(config);
    });
    registerDeviceCategory("i430vx", "chipset");
    
    // i440BX
    registerDeviceType("i440bx", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<I440BX>(config);
    });
    registerDeviceCategory("i440bx", "chipset");
    
    // i440FX
    registerDeviceType("i440fx", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<I440FX>(config);
    });
    registerDeviceCategory("i440fx", "chipset");
    
    // i815
    registerDeviceType("i815", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<I815>(config);
    });
    registerDeviceCategory("i815", "chipset");
    
    // NVIDIA nForce
    registerDeviceType("nforce", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<NForce>(config);
    });
    registerDeviceCategory("nforce", "chipset");
}

void DeviceFactory::RegisterVideoDevices()
{
    // Standard VGA
    registerDeviceType("vga", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<VGA>(config);
    });
    registerDeviceCategory("vga", "video");
    
    // SVGA
    registerDeviceType("svga", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<SVGA>(config);
    });
    registerDeviceCategory("svga", "video");
    
    // S3 ViRGE
    registerDeviceType("s3virge", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<S3ViRGE>(config);
    });
    registerDeviceCategory("s3virge", "video");
    
    // S3 Trio64
    registerDeviceType("s3trio64", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<S3Trio64>(config);
    });
    registerDeviceCategory("s3trio64", "video");
    
    // GeForce3
    registerDeviceType("geforce3", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<GeForce3>(config);
    });
    registerDeviceCategory("geforce3", "video");
}

void DeviceFactory::RegisterSoundDevices()
{
    // Sound Blaster 16
    registerDeviceType("sb16", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<SoundBlaster16>(config);
    });
    registerDeviceCategory("sb16", "sound");
    
    // Ensoniq ES1370
    registerDeviceType("es1370", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<ES1370>(config);
    });
    registerDeviceCategory("es1370", "sound");
    
    // Ensoniq ES1371
    registerDeviceType("es1371", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<ES1371>(config);
    });
    registerDeviceCategory("es1371", "sound");
}

void DeviceFactory::RegisterStorageDevices()
{
    // IDE controller
    registerDeviceType("ide", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<IDE>(config);
    });
    registerDeviceCategory("ide", "storage");
    
    // SCSI controller
    registerDeviceType("scsi", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<SCSI>(config);
    });
    registerDeviceCategory("scsi", "storage");
    
    // Floppy controller
    registerDeviceType("floppy", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<Floppy>(config);
    });
    registerDeviceCategory("floppy", "storage");
}

void DeviceFactory::RegisterInputDevices()
{
    // Keyboard controller
    registerDeviceType("keyboard", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<Keyboard>(config);
    });
    registerDeviceCategory("keyboard", "input");
    
    // Mouse controller
    registerDeviceType("mouse", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<Mouse>(config);
    });
    registerDeviceCategory("mouse", "input");
    
    // Joystick controller
    registerDeviceType("joystick", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<Joystick>(config);
    });
    registerDeviceCategory("joystick", "input");
}

void DeviceFactory::RegisterNetworkDevices()
{
    // NE2000 network adapter
    registerDeviceType("ne2000", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<NE2000>(config);
    });
    registerDeviceCategory("ne2000", "network");
    
    // RTL8139 network adapter
    registerDeviceType("rtl8139", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<RTL8139>(config);
    });
    registerDeviceCategory("rtl8139", "network");
}

void DeviceFactory::RegisterBusDevices()
{
    // PCI bus controller
    registerDeviceType("pci", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<PCIBus>(config);
    });
    registerDeviceCategory("pci", "bus");
    
    // ISA bus controller
    registerDeviceType("isa", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<ISABus>(config);
    });
    registerDeviceCategory("isa", "bus");
    
    // USB controller
    registerDeviceType("usb", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<USBController>(config);
    });
    registerDeviceCategory("usb", "bus");
}

void DeviceFactory::RegisterPeripheralDevices()
{
    // Serial port
    registerDeviceType("serial", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<SerialPort>(config);
    });
    registerDeviceCategory("serial", "peripheral");
    
    // Parallel port
    registerDeviceType("parallel", [](const DeviceConfig& config) -> std::shared_ptr<Device> {
        return std::make_shared<ParallelPort>(config);
    });
    registerDeviceCategory("parallel", "peripheral");
}