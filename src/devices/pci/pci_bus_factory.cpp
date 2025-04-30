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

#include "devices/pci/pci_bus_factory.h"
#include "devices/pci/pci_device.h"
#include "devices/pci/pci_host.h"
#include "devices/machine/sis630_host.h"
#include "devices/machine/sis5513_ide.h"
#include "devices/machine/sis7001_usb.h"
#include "devices/machine/sis7018_audio.h"
#include "devices/video/sis_vga.h"
#include "config_manager.h"
#include "logger.h"

namespace x86emu {

std::unique_ptr<PCIBus> PCIBusFactory::createPCIBus(PCIBusType type, uint32_t ramSize)
{
    auto bus = std::make_unique<PCIBus>();
    std::shared_ptr<PCIHostDevice> host;
    
    switch (type) {
        case PCIBusType::SIS_630:
            // Create SiS 630 chipset
            Logger::GetInstance()->info("Creating SiS 630 chipset PCI bus");
            auto sis630host = std::make_shared<SiS630HostDevice>(ramSize);
            host = sis630host;
            bus->addDevice(0, 0, host);
            
            // Add IDE controller at PCI 0:1
            auto ide = std::make_shared<SiS5513IdeDevice>();
            bus->addDevice(0, 1, ide);
            
            // Add USB controller with 2 ports at PCI 0:2
            auto usb1 = std::make_shared<SiS7001UsbDevice>(2);
            bus->addDevice(0, 2, usb1);
            
            // Add second USB controller with 3 ports at PCI 0:3
            auto usb2 = std::make_shared<SiS7001UsbDevice>(3);
            bus->addDevice(0, 3, usb2);
            
            // Add audio controller at PCI 0:4
            auto audio = std::make_shared<SiS7018AudioDevice>();
            bus->addDevice(0, 4, audio);
            
            // Add integrated VGA at PCI 1:0 (function 1 of host bridge)
            auto vga = std::make_shared<SisVgaAdapter>();
            bus->addDevice(1, 0, vga);
            
            // Connect VGA to host bridge for proper integration
            sis630host->connectVGA(vga);
            
            bus->setHostDevice(host);
            break;
            
        case PCIBusType::INTEL_440BX:
            // TODO: Implement Intel 440BX chipset
            Logger::GetInstance()->error("Intel 440BX chipset not implemented yet");
            return nullptr;
            
        default:
            Logger::GetInstance()->error("Unknown PCI bus type");
            return nullptr;
    }
    
    if (!bus->initialize()) {
        Logger::GetInstance()->error("Failed to initialize PCI bus");
        return nullptr;
    }
    
    return bus;
}

PCIBusType PCIBusFactory::getPCIBusTypeFromConfig(ConfigManager* config)
{
    std::string chipsetName = config->getString("motherboard", "chipset", "sis630");
    
    if (chipsetName == "sis630" || chipsetName == "sis730") {
        return PCIBusType::SIS_630;
    }
    else if (chipsetName == "i440bx") {
        return PCIBusType::INTEL_440BX;
    }
    
    Logger::GetInstance()->warn("Unknown chipset '%s', defaulting to SiS 630", chipsetName.c_str());
    return PCIBusType::SIS_630;
}

} // namespace x86emu