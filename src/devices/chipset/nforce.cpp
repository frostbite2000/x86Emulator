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

#include "devices/chipset/nforce/nforce.h"
#include "devices/cpu/cpu.h"
#include "devices/bus/pci/pci_bus.h"
#include "log.h"

#include <cstring>
#include <cmath>
#include <algorithm>

// Default SPD data for DDR memory
static const uint8_t DEFAULT_SPD_DATA[] = {
    0x80, 0x08, 0x07, 0x0D, 0x0A, 0x01, 0x40, 0x00, 0x04, 0x75, 0x75, 0x00, 0x82, 0x10, 0x00, 0x01,
    0x0E, 0x04, 0x0C, 0x01, 0x02, 0x20, 0xC0, 0xA0, 0x75, 0x00, 0x00, 0x50, 0x3C, 0x50, 0x2D, 0x40,
    0x90, 0x90, 0x50, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x4B, 0x30, 0x32, 0x75, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEA,
    0xAD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x59, 0x4D, 0x44, 0x35, 0x33, 0x32,
    0x4D, 0x36, 0x34, 0x36, 0x43, 0x36, 0x2D, 0x48, 0x20, 0x20, 0x20
};

// SMBus Logger implementation
SMBusLogger::SMBusLogger() {
    memset(buffer, 0, sizeof(buffer));
}

int SMBusLogger::executeCommand(int command, int rw, int data) {
    if (rw == 1) {
        LOG_DEBUG("SMBus: read from %02X R %02X\n", command, buffer[command]);
        return buffer[command];
    }
    buffer[command] = static_cast<uint8_t>(data);
    LOG_DEBUG("SMBus: write to %02X W %02X\n", command, data);
    return 0;
}

// SMBus ROM implementation
SMBusROM::SMBusROM(const uint8_t* data, int size) : buffer(data), bufferSize(size) {
}

int SMBusROM::executeCommand(int command, int rw, int data) {
    if ((rw == 1) && (command < bufferSize) && (buffer != nullptr)) {
        LOG_DEBUG("SMBus ROM: read from %02X %02X\n", command, buffer[command]);
        return buffer[command];
    }
    return 0;
}

// AS99127F implementation
AS99127F::AS99127F() {
    memset(buffer, 0, sizeof(buffer));
    
    // Set up default values for hardware monitoring
    buffer[0x20] = 0x70; // multiplied by 0x10
    buffer[0x2] = 0x7e;  // multiplied by 0x10
    buffer[0x23] = 0x96; // multiplied by 0x540 then divided by 0x32
    buffer[0x24] = 0x9e; // multiplied by 0x260 then divided by 0xa
    buffer[0x4f] = 0x12; // Set in MAME initialization
}

int AS99127F::executeCommand(int command, int rw, int data) {
    if (rw == 1) {
        LOG_DEBUG("AS99127F: read from %02X R %02X\n", command, buffer[command]);
        return buffer[command];
    }
    buffer[command] = static_cast<uint8_t>(data);
    LOG_DEBUG("AS99127F: write to %02X W %02X\n", command, data);
    return 0;
}

// AS99127F Sensor 2 implementation
AS99127FSensor2::AS99127FSensor2() {
    memset(buffer, 0, sizeof(buffer));
}

int AS99127FSensor2::executeCommand(int command, int rw, int data) {
    if (rw == 1) {
        LOG_DEBUG("AS99127F Sensor2: read from %02X R %02X\n", command, buffer[command]);
        return buffer[command];
    }
    buffer[command] = static_cast<uint8_t>(data);
    LOG_DEBUG("AS99127F Sensor2: write to %02X W %02X\n", command, data);
    return 0;
}

// AS99127F Sensor 3 implementation
AS99127FSensor3::AS99127FSensor3() {
    memset(buffer, 0, sizeof(buffer));
}

int AS99127FSensor3::executeCommand(int command, int rw, int data) {
    if (rw == 1) {
        LOG_DEBUG("AS99127F Sensor3: read from %02X R %02X\n", command, buffer[command]);
        return buffer[command];
    }
    buffer[command] = static_cast<uint8_t>(data);
    LOG_DEBUG("AS99127F Sensor3: write to %02X W %02X\n", command, data);
    return 0;
}

// nForce chipset implementation
NForce::NForce(const DeviceConfig& config)
    : Chipset(config),
      m_cpu(nullptr),
      m_pciBus(nullptr),
      m_cpuBridge(nullptr),
      m_memoryCtrl(nullptr),
      m_isaBridge(nullptr),
      m_smbusCtrl(nullptr),
      m_ethCtrl(nullptr),
      m_apu(nullptr),
      m_ac97Audio(nullptr),
      m_ideCtrl(nullptr),
      m_agpBridge(nullptr)
{
    // USB controllers are initialized to nullptr in the array constructor
}

bool NForce::Initialize() {
    if (IsInitialized()) {
        return true;
    }
    
    // Create nForce chipset components
    
    // CPU Bridge (North Bridge)
    DeviceConfig cpuBridgeConfig = m_config;
    cpuBridgeConfig.set("vendor_id", 0x10de);
    cpuBridgeConfig.set("device_id", 0x01a4);
    cpuBridgeConfig.set("revision_id", 0xb2);
    cpuBridgeConfig.set("class_code", 0x060000);
    cpuBridgeConfig.set("subsystem_vendor_id", 0x10de);
    cpuBridgeConfig.set("subsystem_id", 0x0c11);
    
    m_cpuBridge = new NFORCE_CpuBridge(cpuBridgeConfig);
    if (!m_cpuBridge->Initialize()) {
        LOG_ERROR("Failed to initialize nForce CPU Bridge\n");
        return false;
    }
    
    // Memory Controller
    DeviceConfig memConfig = m_config;
    memConfig.set("vendor_id", 0x10de);
    memConfig.set("device_id", 0x01ac);
    memConfig.set("revision_id", 0xb2);
    memConfig.set("class_code", 0x050000);
    memConfig.set("subsystem_vendor_id", 0x1043);
    memConfig.set("subsystem_id", 0x0c11);
    
    m_memoryCtrl = new NFORCE_Memory(memConfig);
    if (!m_memoryCtrl->Initialize()) {
        LOG_ERROR("Failed to initialize nForce Memory Controller\n");
        return false;
    }
    
    // ISA Bridge (South Bridge / MCP-D)
    DeviceConfig isaConfig = m_config;
    isaConfig.set("vendor_id", 0x10de);
    isaConfig.set("device_id", 0x01b2);
    isaConfig.set("revision_id", 0xb4);
    isaConfig.set("class_code", 0x060100);
    isaConfig.set("subsystem_vendor_id", 0x1043);
    isaConfig.set("subsystem_id", 0x0c11);
    
    m_isaBridge = new NFORCE_ISABridge(isaConfig);
    if (!m_isaBridge->Initialize()) {
        LOG_ERROR("Failed to initialize nForce ISA Bridge\n");
        return false;
    }
    
    // Set CPU interface for the ISA bridge
    if (m_cpu) {
        m_isaBridge->SetCPUInterface(m_cpu);
    }
    
    // SMBus Controller
    DeviceConfig smbusConfig = m_config;
    smbusConfig.set("vendor_id", 0x10de);
    smbusConfig.set("device_id", 0x01b4);
    smbusConfig.set("revision_id", 0xc1);
    smbusConfig.set("class_code", 0x0c0500);
    smbusConfig.set("subsystem_vendor_id", 0x1043);
    smbusConfig.set("subsystem_id", 0x0c11);
    
    m_smbusCtrl = new NFORCE_SMBus(smbusConfig);
    if (!m_smbusCtrl->Initialize()) {
        LOG_ERROR("Failed to initialize nForce SMBus Controller\n");
        return false;
    }
    
    // USB Controllers
    for (int i = 0; i < 2; i++) {
        DeviceConfig usbConfig = m_config;
        usbConfig.set("vendor_id", 0x10de);
        usbConfig.set("device_id", 0x01c2);
        usbConfig.set("revision_id", 0xc3);
        usbConfig.set("class_code", 0x0c0310);
        usbConfig.set("subsystem_vendor_id", 0x1043);
        usbConfig.set("subsystem_id", 0x0c11);
        
        m_usbCtrl[i] = new NFORCE_USB(usbConfig);
        if (!m_usbCtrl[i]->Initialize()) {
            LOG_ERROR("Failed to initialize nForce USB Controller %d\n", i);
            return false;
        }
    }
    
    // Ethernet Controller
    DeviceConfig ethConfig = m_config;
    ethConfig.set("vendor_id", 0x10de);
    ethConfig.set("device_id", 0x01c3);
    ethConfig.set("revision_id", 0);
    ethConfig.set("class_code", 0x020000);
    ethConfig.set("subsystem_vendor_id", 0x1043);
    ethConfig.set("subsystem_id", 0x0c11);
    
    m_ethCtrl = new NFORCE_Ethernet(ethConfig);
    if (!m_ethCtrl->Initialize()) {
        LOG_ERROR("Failed to initialize nForce Ethernet Controller\n");
        return false;
    }
    
    // Audio Processing Unit (APU)
    DeviceConfig apuConfig = m_config;
    apuConfig.set("vendor_id", 0x10de);
    apuConfig.set("device_id", 0x01b0);
    apuConfig.set("revision_id", 0);
    apuConfig.set("class_code", 0x048000);
    apuConfig.set("subsystem_vendor_id", 0x1043);
    apuConfig.set("subsystem_id", 0x0c11);
    
    m_apu = new NFORCE_APU(apuConfig);
    if (!m_apu->Initialize()) {
        LOG_ERROR("Failed to initialize nForce APU\n");
        return false;
    }
    
    // AC'97 Audio Controller
    DeviceConfig ac97Config = m_config;
    ac97Config.set("vendor_id", 0x10de);
    ac97Config.set("device_id", 0x01b1);
    ac97Config.set("revision_id", 0xc2);
    ac97Config.set("class_code", 0x040100);
    ac97Config.set("subsystem_vendor_id", 0x1043);
    ac97Config.set("subsystem_id", 0x8384);
    
    m_ac97Audio = new NFORCE_AC97Audio(ac97Config);
    if (!m_ac97Audio->Initialize()) {
        LOG_ERROR("Failed to initialize nForce AC'97 Audio Controller\n");
        return false;
    }
    
    // IDE Controller
    DeviceConfig ideConfig = m_config;
    ideConfig.set("vendor_id", 0x10de);
    ideConfig.set("device_id", 0x01bc);
    ideConfig.set("revision_id", 0xc3);
    ideConfig.set("class_code", 0x01018a);
    ideConfig.set("subsystem_vendor_id", 0x1043);
    ideConfig.set("subsystem_id", 0x0c11);
    
    m_ideCtrl = new NFORCE_IDE(ideConfig);
    if (!m_ideCtrl->Initialize()) {
        LOG_ERROR("Failed to initialize nForce IDE Controller\n");
        return false;
    }
    
    // AGP to PCI Bridge
    DeviceConfig agpConfig = m_config;
    agpConfig.set("vendor_id", 0x10de);
    agpConfig.set("device_id", 0x01b7);
    agpConfig.set("revision_id", 0xb2);
    agpConfig.set("class_code", 0x060400);
    agpConfig.set("subsystem_vendor_id", 0);
    agpConfig.set("subsystem_id", 0);
    
    m_agpBridge = new NFORCE_AGPBridge(agpConfig);
    if (!m_agpBridge->Initialize()) {
        LOG_ERROR("Failed to initialize nForce AGP Bridge\n");
        return false;
    }
    
    // Register SMBus devices
    RegisterDefaultSMBusDevices();
    
    // Find PCI bus
    m_pciBus = dynamic_cast<PCIBus*>(m_parent);
    if (!m_pciBus) {
        LOG_ERROR("NForce chipset requires a PCI bus parent\n");
        return false;
    }
    
    // Add all devices to the PCI bus at their respective addresses
    m_pciBus->AddDevice(0, 0, m_cpuBridge);  // 00:00.0
    m_pciBus->AddDevice(0, 1, m_memoryCtrl); // 00:01.0
    m_pciBus->AddDevice(1, 0, m_isaBridge);  // 01:00.0
    m_pciBus->AddDevice(1, 1, m_smbusCtrl);  // 01:01.0
    m_pciBus->AddDevice(2, 0, m_usbCtrl[0]); // 02:00.0
    m_pciBus->AddDevice(3, 0, m_usbCtrl[1]); // 03:00.0
    m_pciBus->AddDevice(4, 0, m_ethCtrl);    // 04:00.0
    m_pciBus->AddDevice(5, 0, m_apu);        // 05:00.0
    m_pciBus->AddDevice(6, 0, m_ac97Audio);  // 06:00.0
    m_pciBus->AddDevice(9, 0, m_ideCtrl);    // 09:00.0
    m_pciBus->AddDevice(0x1E, 0, m_agpBridge); // 1E:00.0
    
    SetInitialized(true);
    return true;
}

void NForce::Reset() {
    if (!IsInitialized()) {
        return;
    }
    
    m_cpuBridge->Reset();
    m_memoryCtrl->Reset();
    m_isaBridge->Reset();
    m_smbusCtrl->Reset();
    
    for (auto& usb : m_usbCtrl) {
        if (usb) {
            usb->Reset();
        }
    }
    
    m_ethCtrl->Reset();
    m_apu->Reset();
    m_ac97Audio->Reset();
    m_ideCtrl->Reset();
    m_agpBridge->Reset();
}

void NForce::SetCPU(CPU* cpu) {
    m_cpu = cpu;
    
    if (m_isaBridge) {
        m_isaBridge->SetCPUInterface(cpu);
    }
}

void NForce::SetBIOS(uint8_t* biosRom) {
    // Forward to CPU bridge which handles the BIOS mapping
}

void NForce::SetRAM(void* ramPtr, uint32_t ramSizeMB) {
    if (m_cpuBridge) {
        m_cpuBridge->SetRAMSize(ramSizeMB);
    }
    
    if (m_memoryCtrl) {
        m_memoryCtrl->SetupRAM(ramPtr, ramSizeMB);
    }
}

void NForce::RegisterDefaultSMBusDevices() {
    // Create all the SMBus devices
    m_spdRom = std::make_unique<SMBusROM>(DEFAULT_SPD_DATA, sizeof(DEFAULT_SPD_DATA));
    m_smbusLogger1 = std::make_unique<SMBusLogger>();
    m_smbusLogger2 = std::make_unique<SMBusLogger>();
    m_smbusLogger3 = std::make_unique<SMBusLogger>();
    m_as99127f = std::make_unique<AS99127F>();
    m_as99127fSensor2 = std::make_unique<AS99127FSensor2>();
    m_as99127fSensor3 = std::make_unique<AS99127FSensor3>();
    
    // Register them with the SMBus controller
    m_smbusCtrl->RegisterSMBusDevice(0, 0x50, m_spdRom.get());          // SMBus 0, Address 0x50: SPD ROM
    m_smbusCtrl->RegisterSMBusDevice(0, 0x51, m_smbusLogger1.get());    // SMBus 0, Address 0x51: Logger
    m_smbusCtrl->RegisterSMBusDevice(0, 0x52, m_smbusLogger2.get());    // SMBus 0, Address 0x52: Logger
    m_smbusCtrl->RegisterSMBusDevice(1, 0x08, m_smbusLogger3.get());    // SMBus 1, Address 0x08: Logger
    m_smbusCtrl->RegisterSMBusDevice(1, 0x2D, m_as99127f.get());        // SMBus 1, Address 0x2D: AS99127F
    m_smbusCtrl->RegisterSMBusDevice(1, 0x48, m_as99127fSensor2.get()); // SMBus 1, Address 0x48: AS99127F Sensor 2
    m_smbusCtrl->RegisterSMBusDevice(1, 0x49, m_as99127fSensor3.get()); // SMBus 1, Address 0x49: AS99127F Sensor 3
}