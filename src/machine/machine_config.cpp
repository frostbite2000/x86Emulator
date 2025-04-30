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

#include "machine/machine_config.h"
#include "logger.h"

namespace x86emu {

MachineConfig::MachineConfig(const std::string& name, const std::string& description)
    : m_name(name)
    , m_description(description)
{
    // Initialize with default values
    m_hardwareSpec.chipset = ChipsetType::UNKNOWN;
    m_hardwareSpec.formFactor = FormFactor::UNKNOWN;
    m_hardwareSpec.maxRamMB = 0;
    m_hardwareSpec.hasOnboardAudio = false;
    m_hardwareSpec.hasOnboardVideo = false;
    m_hardwareSpec.hasOnboardLan = false;
    m_hardwareSpec.numIdeChannels = 0;
    m_hardwareSpec.hasFloppyController = false;
    m_hardwareSpec.numSerialPorts = 0;
    m_hardwareSpec.numParallelPorts = 0;
    m_hardwareSpec.numUsbPorts = 0;
    m_hardwareSpec.hasPs2Ports = false;
}

std::string MachineConfig::getChipsetName() const
{
    switch (m_hardwareSpec.chipset) {
        case ChipsetType::SIS_630:
            return "SiS 630";
        case ChipsetType::SIS_730:
            return "SiS 730";
        case ChipsetType::INTEL_440BX:
            return "Intel 440BX";
        default:
            return "Unknown";
    }
}

std::string MachineConfig::getFormFactorName() const
{
    switch (m_hardwareSpec.formFactor) {
        case FormFactor::ATX:
            return "ATX";
        case FormFactor::BABY_AT:
            return "Baby AT";
        case FormFactor::MICRO_ATX:
            return "Micro ATX";
        case FormFactor::MINI_ITX:
            return "Mini ITX";
        default:
            return "Unknown";
    }
}

MachineManager& MachineManager::getInstance()
{
    static MachineManager instance;
    return instance;
}

void MachineManager::initialize()
{
    // Add available machine configurations
    addMachine(createZidaV630E());
    addMachine(createShuttleMS11());
    
    Logger::GetInstance()->info("Machine manager initialized with %d machine configurations", m_machines.size());
}

void MachineManager::addMachine(std::shared_ptr<MachineConfig> machine)
{
    if (machine) {
        m_machines.push_back(machine);
    }
}

std::vector<std::shared_ptr<MachineConfig>> MachineManager::getAvailableMachines() const
{
    return m_machines;
}

std::shared_ptr<MachineConfig> MachineManager::getMachineByName(const std::string& name) const
{
    for (const auto& machine : m_machines) {
        if (machine->getName() == name) {
            return machine;
        }
    }
    return nullptr;
}

std::shared_ptr<MachineConfig> MachineManager::createZidaV630E()
{
    auto machine = std::make_shared<MachineConfig>(
        "Zida_V630E",
        "Zida V630E Baby AT motherboard with SiS 630 chipset"
    );
    
    // Set hardware specification
    MachineConfig::HardwareSpec spec;
    spec.chipset = MachineConfig::ChipsetType::SIS_630;
    spec.formFactor = MachineConfig::FormFactor::BABY_AT;
    spec.maxRamMB = 512;  // Supports up to 512MB
    spec.hasOnboardAudio = true;  // AC97 audio
    spec.hasOnboardVideo = true;  // SiS 630 integrated
    spec.hasOnboardLan = false;   // No onboard LAN
    spec.numIdeChannels = 2;      // Primary and secondary IDE
    spec.hasFloppyController = true;
    spec.numSerialPorts = 2;
    spec.numParallelPorts = 1;
    spec.numUsbPorts = 2;         // 2 USB 1.1 ports
    spec.hasPs2Ports = true;      // PS/2 keyboard and mouse
    
    machine->setHardwareSpec(spec);
    
    // Set BIOS information
    MachineConfig::BiosInfo bios;
    bios.manufacturer = "Zida";
    bios.version = "1.08E";
    bios.date = "02/20/2002";
    bios.filename = "v630108e.bin";
    bios.size = 256 * 1024;       // 256KB
    bios.address = 0x000F0000;    // F0000-FFFFF
    
    machine->setBiosInfo(bios);
    
    // Set default configuration options
    std::map<std::string, std::string> defaultOptions;
    defaultOptions["cpu"] = "pentium3";
    defaultOptions["cpu_speed"] = "800";  // 800 MHz
    defaultOptions["memory"] = "256";     // 256MB RAM
    defaultOptions["boot_order"] = "fdc,hdc,cdrom";
    defaultOptions["vga"] = "integrated";
    defaultOptions["sound"] = "integrated";
    defaultOptions["floppy_a"] = "1.44M";
    defaultOptions["floppy_b"] = "none";
    
    machine->setDefaultOptions(defaultOptions);
    
    return machine;
}

std::shared_ptr<MachineConfig> MachineManager::createShuttleMS11()
{
    auto machine = std::make_shared<MachineConfig>(
        "Shuttle_MS11",
        "Shuttle MS11 motherboard with SiS 630 chipset"
    );
    
    // Set hardware specification
    MachineConfig::HardwareSpec spec;
    spec.chipset = MachineConfig::ChipsetType::SIS_630;
    spec.formFactor = MachineConfig::FormFactor::MICRO_ATX;
    spec.maxRamMB = 1024;  // Supports up to 1GB
    spec.hasOnboardAudio = true;  // AC97 audio
    spec.hasOnboardVideo = true;  // SiS 630 integrated
    spec.hasOnboardLan = true;    // 10/100 Ethernet
    spec.numIdeChannels = 2;      // Primary and secondary IDE
    spec.hasFloppyController = true;
    spec.numSerialPorts = 2;
    spec.numParallelPorts = 1;
    spec.numUsbPorts = 4;         // 4 USB 1.1 ports
    spec.hasPs2Ports = true;      // PS/2 keyboard and mouse
    
    machine->setHardwareSpec(spec);
    
    // Set BIOS information
    MachineConfig::BiosInfo bios;
    bios.manufacturer = "Shuttle";
    bios.version = "S11D";
    bios.date = "05/15/2002";
    bios.filename = "ms11s11d.bin";
    bios.size = 256 * 1024;       // 256KB
    bios.address = 0x000F0000;    // F0000-FFFFF
    
    machine->setBiosInfo(bios);
    
    // Set default configuration options
    std::map<std::string, std::string> defaultOptions;
    defaultOptions["cpu"] = "pentium3";
    defaultOptions["cpu_speed"] = "1000";  // 1 GHz
    defaultOptions["memory"] = "512";      // 512MB RAM
    defaultOptions["boot_order"] = "cdrom,hdc,fdc";
    defaultOptions["vga"] = "integrated";
    defaultOptions["sound"] = "integrated";
    defaultOptions["floppy_a"] = "1.44M";
    defaultOptions["floppy_b"] = "none";
    defaultOptions["network"] = "rtl8139";
    
    machine->setDefaultOptions(defaultOptions);
    
    return machine;
}

} // namespace x86emu