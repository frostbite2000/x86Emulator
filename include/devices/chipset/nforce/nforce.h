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

#ifndef X86EMULATOR_NFORCE_H
#define X86EMULATOR_NFORCE_H

#include "devices/device.h"
#include "devices/pci/pci_device.h"
#include "devices/chipset/chipset_common.h"
#include "devices/bus/pci/pci_bus.h"
#include "devices/interrupt/pic.h"
#include "devices/timer/pit.h"
#include "devices/rtc/ds12885.h"
#include "devices/dma/i8237.h"

#include <vector>
#include <functional>
#include <memory>

class NForce;

class SMBusDevice {
public:
    virtual ~SMBusDevice() = default;
    virtual int executeCommand(int command, int rw, int data) = 0;
};

// SMBus logger device - for debugging
class SMBusLogger : public SMBusDevice {
public:
    SMBusLogger();
    ~SMBusLogger() override = default;
    
    int executeCommand(int command, int rw, int data) override;
    uint8_t* getBuffer() { return buffer; }
    
private:
    uint8_t buffer[0xff];
};

// SMBus ROM device - used for SPD on DIMM modules
class SMBusROM : public SMBusDevice {
public:
    SMBusROM(const uint8_t* data, int size);
    ~SMBusROM() override = default;
    
    int executeCommand(int command, int rw, int data) override;
    
private:
    const uint8_t* buffer;
    int bufferSize;
};

// AS99127F hardware monitoring chip
class AS99127F : public SMBusDevice {
public:
    AS99127F();
    ~AS99127F() override = default;
    
    int executeCommand(int command, int rw, int data) override;
    uint8_t* getBuffer() { return buffer; }
    
private:
    uint8_t buffer[0xff];
};

class AS99127FSensor2 : public SMBusDevice {
public:
    AS99127FSensor2();
    ~AS99127FSensor2() override = default;
    
    int executeCommand(int command, int rw, int data) override;
    uint8_t* getBuffer() { return buffer; }
    
private:
    uint8_t buffer[0xff];
};

class AS99127FSensor3 : public SMBusDevice {
public:
    AS99127FSensor3();
    ~AS99127FSensor3() override = default;
    
    int executeCommand(int command, int rw, int data) override;
    uint8_t* getBuffer() { return buffer; }
    
private:
    uint8_t buffer[0xff];
};

// nForce CPU Bridge (aka CRUSH11)
class NFORCE_CpuBridge : public PCIDevice {
public:
    explicit NFORCE_CpuBridge(const DeviceConfig& config);
    ~NFORCE_CpuBridge() override = default;
    
    bool Initialize() override;
    void Reset() override;
    std::string GetName() const override { return "NVIDIA nForce CPU Bridge"; }
    std::string GetType() const override { return "chipset"; }
    
    // PCI config space access
    uint32_t ReadConfig(uint8_t reg, int size) override;
    void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    
    // Access to allocated RAM
    void SetRAMSize(uint32_t size);
    uint32_t GetRAMSize() const { return m_ramSize; }
    
    // Memory mapping
    void MapMemoryWindows();
    void* GetRAMPtr() const { return nullptr; } // Implemented by owner
    
private:
    void ResetAllMappings();
    
    uint32_t m_ramSize; // In MB
    uint8_t* m_biosrom;
    uint32_t m_ramSizeReg;
};

// nForce Memory Controller
class NFORCE_Memory : public PCIDevice {
public:
    explicit NFORCE_Memory(const DeviceConfig& config);
    ~NFORCE_Memory() override = default;
    
    bool Initialize() override;
    void Reset() override;
    std::string GetName() const override { return "NVIDIA nForce Memory Controller"; }
    std::string GetType() const override { return "chipset"; }
    
    // PCI config space access
    uint32_t ReadConfig(uint8_t reg, int size) override;
    void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    
    void SetupRAM(void* ramPtr, uint32_t ramSize);
    
private:
    int m_ddrRamSize;              // RAM size in MB
    std::vector<uint32_t> m_ram;   // RAM buffer
    NFORCE_CpuBridge* m_hostBridge;
};

// nForce MCP-D South Bridge - ISA/LPC Bridge
class NFORCE_ISABridge : public PCIDevice, public IRQController {
public:
    explicit NFORCE_ISABridge(const DeviceConfig& config);
    ~NFORCE_ISABridge() override = default;
    
    bool Initialize() override;
    void Reset() override;
    std::string GetName() const override { return "NVIDIA nForce ISA Bridge"; }
    std::string GetType() const override { return "chipset"; }
    
    // PCI config space access
    uint32_t ReadConfig(uint8_t reg, int size) override;
    void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    
    // Memory and I/O access
    uint32_t ReadMemory(uint32_t address, int size) override;
    void WriteMemory(uint32_t address, uint32_t value, int size) override;
    uint32_t ReadIO(uint16_t port, int size) override;
    void WriteIO(uint16_t port, uint32_t value, int size) override;
    
    // IRQ handling
    uint32_t AcknowledgeIRQ() override;
    void AssertIRQ(int irqNum) override;
    void DeassertIRQ(int irqNum) override;
    
    // ACPI interface
    uint32_t ReadACPI(uint32_t offset, uint32_t mask);
    void WriteACPI(uint32_t offset, uint32_t data, uint32_t mask);
    
    // Boot state callback for BIOS POST codes
    void BootStateCallback(uint8_t data);
    
    // Methods for interconnection with other devices
    void SetCPUInterface(CPU* cpu) { m_cpu = cpu; }
    
    // Timer interrupt handling
    void PITChannel0Changed(bool state);
    void PITChannel1Changed(bool state);
    void PITChannel2Changed(bool state);
    
private:
    // ACPI registers
    uint16_t m_pm1Status;
    uint16_t m_pm1Enable;
    uint16_t m_pm1Control;
    uint16_t m_pm1Timer;
    uint16_t m_gpe0Status;
    uint16_t m_gpe0Enable;
    uint16_t m_globalSmiControl;
    uint8_t m_smiCommandPort;
    
    // GPIO registers
    uint8_t m_gpioMode[26];
    
    // DMA controller registers
    uint8_t m_dmaPage[0x10];
    uint8_t m_dmaHighByte;
    int m_dmaChannel;
    uint32_t m_pageOffset;
    
    // Speaker control
    uint8_t m_speaker;
    bool m_refresh;
    uint8_t m_pitOut2;
    uint8_t m_spkrData;
    uint8_t m_channelCheck;
    
    // Associated devices
    PIC8259* m_pic1;
    PIC8259* m_pic2;
    I8237DMA* m_dma1;
    I8237DMA* m_dma2;
    PIT8254* m_pit;
    DS12885* m_rtc;
    
    CPU* m_cpu;
    bool m_interruptOutputState;
    bool m_dmaEop;
};

// nForce SMBus Controller
class NFORCE_SMBus : public PCIDevice {
public:
    explicit NFORCE_SMBus(const DeviceConfig& config);
    ~NFORCE_SMBus() override = default;
    
    bool Initialize() override;
    void Reset() override;
    std::string GetName() const override { return "NVIDIA nForce SMBus Controller"; }
    std::string GetType() const override { return "chipset"; }
    
    // PCI config space access
    uint32_t ReadConfig(uint8_t reg, int size) override;
    void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    
    // I/O access
    uint32_t ReadIO(uint16_t port, int size) override;
    void WriteIO(uint16_t port, uint32_t value, int size) override;
    
    // Register SMBus devices
    void RegisterSMBusDevice(int bus, int address, SMBusDevice* device);
    
private:
    struct SMBusState {
        int status;
        int control;
        int address;
        int data;
        int command;
        int rw;
        SMBusDevice* devices[128];
        uint32_t words[256 / 4];
    };
    
    SMBusState m_smbusState[2];
    
    uint32_t SMBusRead(int bus, uint32_t offset, uint32_t mask);
    void SMBusWrite(int bus, uint32_t offset, uint32_t data, uint32_t mask);
    
    NFORCE_ISABridge* m_isaBridge;
};

// nForce USB Controller (OHCI)
class NFORCE_USB : public PCIDevice {
public:
    explicit NFORCE_USB(const DeviceConfig& config);
    ~NFORCE_USB() override = default;
    
    bool Initialize() override;
    void Reset() override;
    std::string GetName() const override { return "NVIDIA nForce USB Controller"; }
    std::string GetType() const override { return "usb"; }
    
    // PCI config space access
    uint32_t ReadConfig(uint8_t reg, int size) override;
    void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    
    // Memory access
    uint32_t ReadMemory(uint32_t address, int size) override;
    void WriteMemory(uint32_t address, uint32_t value, int size) override;

private:
    // We'll implement a minimal OHCI controller for now
    uint32_t m_ohciRegs[256];
    NFORCE_ISABridge* m_isaBridge;
};

// nForce Ethernet Controller
class NFORCE_Ethernet : public PCIDevice {
public:
    explicit NFORCE_Ethernet(const DeviceConfig& config);
    ~NFORCE_Ethernet() override = default;
    
    bool Initialize() override;
    void Reset() override;
    std::string GetName() const override { return "NVIDIA nForce Ethernet Controller"; }
    std::string GetType() const override { return "network"; }
    
    // PCI config space access
    uint32_t ReadConfig(uint8_t reg, int size) override;
    void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    
    // Memory and I/O access
    uint32_t ReadMemory(uint32_t address, int size) override;
    void WriteMemory(uint32_t address, uint32_t value, int size) override;
    uint32_t ReadIO(uint16_t port, int size) override;
    void WriteIO(uint16_t port, uint32_t value, int size) override;

private:
    // Ethernet controller registers
    std::vector<uint32_t> m_ethRegs;
};

// nForce APU (Audio Processing Unit)
class NFORCE_APU : public PCIDevice {
public:
    explicit NFORCE_APU(const DeviceConfig& config);
    ~NFORCE_APU() override = default;
    
    bool Initialize() override;
    void Reset() override;
    std::string GetName() const override { return "NVIDIA nForce Audio Processing Unit"; }
    std::string GetType() const override { return "sound"; }
    
    // PCI config space access
    uint32_t ReadConfig(uint8_t reg, int size) override;
    void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    
    // Memory access
    uint32_t ReadMemory(uint32_t address, int size) override;
    void WriteMemory(uint32_t address, uint32_t value, int size) override;

private:
    struct APUState {
        uint32_t memory[0x60000 / 4];
        uint32_t gpdspSGAddress;
        uint32_t gpdspSGBlocks;
        uint32_t gpdspAddress;
        uint32_t epdspSGAddress;
        uint32_t epdspSGBlocks;
        uint32_t epdspSGAddress2;
        uint32_t epdspSGBlocks2;
        int voiceNumber;
        uint32_t voicesHeapBlockAddr[1024];
        uint64_t voicesActive[4];
        uint32_t voicedataAddress;
        int voicesFrequency[256];
        int voicesPosition[256];
        int voicesPositionStart[256];
        int voicesPositionEnd[256];
        int voicesPositionIncrement[256];
    };
    
    APUState m_apuState;
    NFORCE_ISABridge* m_isaBridge;
};

// nForce AC'97 Audio Controller
class NFORCE_AC97Audio : public PCIDevice {
public:
    explicit NFORCE_AC97Audio(const DeviceConfig& config);
    ~NFORCE_AC97Audio() override = default;
    
    bool Initialize() override;
    void Reset() override;
    std::string GetName() const override { return "NVIDIA nForce AC'97 Audio Controller"; }
    std::string GetType() const override { return "sound"; }
    
    // PCI config space access
    uint32_t ReadConfig(uint8_t reg, int size) override;
    void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    
    // Memory and I/O access
    uint32_t ReadMemory(uint32_t address, int size) override;
    void WriteMemory(uint32_t address, uint32_t value, int size) override;
    uint32_t ReadIO(uint16_t port, int size) override;
    void WriteIO(uint16_t port, uint32_t value, int size) override;

private:
    struct AC97State {
        uint32_t mixerRegs[0x84 / 4];
        uint32_t controllerRegs[0x40 / 4];
    };
    
    AC97State m_ac97State;
    uint32_t AC97AudioRead(uint32_t offset, uint32_t mask);
    void AC97AudioWrite(uint32_t offset, uint32_t data, uint32_t mask);
    uint32_t AC97AudioIO0Read(uint32_t offset);
    void AC97AudioIO0Write(uint32_t offset, uint32_t data);
    uint32_t AC97AudioIO1Read(uint32_t offset);
    void AC97AudioIO1Write(uint32_t offset, uint32_t data);
};

// nForce IDE Controller
class NFORCE_IDE : public PCIDevice {
public:
    explicit NFORCE_IDE(const DeviceConfig& config);
    ~NFORCE_IDE() override = default;
    
    bool Initialize() override;
    void Reset() override;
    std::string GetName() const override { return "NVIDIA nForce IDE Controller"; }
    std::string GetType() const override { return "storage"; }
    
    // PCI config space access
    uint32_t ReadConfig(uint8_t reg, int size) override;
    void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    
    // I/O access for IDE ports
    uint32_t ReadIO(uint16_t port, int size) override;
    void WriteIO(uint16_t port, uint32_t value, int size) override;

private:
    uint8_t ReadPrimaryCommand(uint16_t port);
    void WritePrimaryCommand(uint16_t port, uint8_t value);
    uint8_t ReadPrimaryControl(uint16_t port);
    void WritePrimaryControl(uint16_t port, uint8_t value);
    uint8_t ReadSecondaryCommand(uint16_t port);
    void WriteSecondaryCommand(uint16_t port, uint8_t value);
    uint8_t ReadSecondaryControl(uint16_t port);
    void WriteSecondaryControl(uint16_t port, uint8_t value);
    uint32_t ReadBusMaster(uint16_t port);
    void WriteBusMaster(uint16_t port, uint32_t value);
    
    NFORCE_ISABridge* m_isaBridge;
    uint16_t m_classRev;
    bool m_priCompatMode;
    bool m_secCompatMode;
};

// nForce AGP to PCI Bridge
class NFORCE_AGPBridge : public PCIDevice {
public:
    explicit NFORCE_AGPBridge(const DeviceConfig& config);
    ~NFORCE_AGPBridge() override = default;
    
    bool Initialize() override;
    void Reset() override;
    std::string GetName() const override { return "NVIDIA nForce AGP to PCI Bridge"; }
    std::string GetType() const override { return "bus"; }
    
    // PCI config space access
    uint32_t ReadConfig(uint8_t reg, int size) override;
    void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    
private:
    uint32_t UnknownRead(uint32_t offset);
    void UnknownWrite(uint32_t offset, uint32_t data);
};

// Main nForce chipset class
class NForce : public Chipset {
public:
    explicit NForce(const DeviceConfig& config);
    ~NForce() override = default;
    
    bool Initialize() override;
    void Reset() override;
    std::string GetName() const override { return "NVIDIA nForce Chipset"; }
    std::string GetType() const override { return "chipset"; }
    
    // Setup methods
    void SetCPU(CPU* cpu);
    void SetBIOS(uint8_t* biosRom);
    void SetRAM(void* ramPtr, uint32_t ramSizeMB);
    
    // Register SMBus devices (called during initialization)
    void RegisterDefaultSMBusDevices();
    
    // Devices in the chipset
    NFORCE_CpuBridge* GetCpuBridge() { return m_cpuBridge; }
    NFORCE_Memory* GetMemoryController() { return m_memoryCtrl; }
    NFORCE_ISABridge* GetISABridge() { return m_isaBridge; }
    NFORCE_SMBus* GetSMBusController() { return m_smbusCtrl; }
    NFORCE_USB* GetUSBController(int index) { return m_usbCtrl[index]; }
    NFORCE_Ethernet* GetEthernetController() { return m_ethCtrl; }
    NFORCE_APU* GetAPU() { return m_apu; }
    NFORCE_AC97Audio* GetAC97Audio() { return m_ac97Audio; }
    NFORCE_IDE* GetIDEController() { return m_ideCtrl; }
    NFORCE_AGPBridge* GetAGPBridge() { return m_agpBridge; }
    
private:
    CPU* m_cpu;
    PCIBus* m_pciBus;
    
    // nForce chipset components
    NFORCE_CpuBridge* m_cpuBridge;
    NFORCE_Memory* m_memoryCtrl;
    NFORCE_ISABridge* m_isaBridge;
    NFORCE_SMBus* m_smbusCtrl;
    NFORCE_USB* m_usbCtrl[2];
    NFORCE_Ethernet* m_ethCtrl;
    NFORCE_APU* m_apu;
    NFORCE_AC97Audio* m_ac97Audio;
    NFORCE_IDE* m_ideCtrl;
    NFORCE_AGPBridge* m_agpBridge;
    
    // SMBus devices
    std::unique_ptr<SMBusROM> m_spdRom;
    std::unique_ptr<SMBusLogger> m_smbusLogger1;
    std::unique_ptr<SMBusLogger> m_smbusLogger2;
    std::unique_ptr<SMBusLogger> m_smbusLogger3;
    std::unique_ptr<AS99127F> m_as99127f;
    std::unique_ptr<AS99127FSensor2> m_as99127fSensor2;
    std::unique_ptr<AS99127FSensor3> m_as99127fSensor3;
};

#endif // X86EMULATOR_NFORCE_H