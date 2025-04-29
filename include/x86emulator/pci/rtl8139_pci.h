#pragma once

#include "x86emulator/pci_device.h"
#include "x86emulator/network/rtl8139.h"
#include <memory>

namespace x86emu {

/**
 * @brief PCI interface for RTL8139 network adapter.
 * 
 * This class provides the PCI bus interface for the RTL8139 network adapter,
 * handling PCI configuration space and memory-mapped I/O.
 */
class RTL8139PCI : public PCIDevice {
public:
    /**
     * Create a RTL8139 PCI device.
     * @param name Unique device name
     */
    explicit RTL8139PCI(const std::string& name);
    
    /**
     * Destructor.
     */
    ~RTL8139PCI() override;

    // Device interface implementation
    bool Initialize() override;
    void Reset() override;
    void Update(uint32_t deltaTime) override;
    
    // PCIDevice interface implementation
    uint32_t ReadConfig(uint8_t reg, int size) override;
    void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    uint32_t ReadMemory(uint32_t address, int size) override;
    void WriteMemory(uint32_t address, uint32_t value, int size) override;
    uint32_t ReadIO(uint16_t address, int size) override;
    void WriteIO(uint16_t address, uint32_t value, int size) override;
    
    /**
     * Set PCI IRQ routing.
     * @param irq IRQ line number
     */
    void SetIRQ(int irq);
    
    /**
     * Get the RTL8139 network adapter.
     * @return Pointer to the RTL8139 adapter
     */
    RTL8139* GetAdapter() const;
    
private:
    // RTL8139 network adapter
    std::unique_ptr<RTL8139> m_adapter;
    
    // PCI configuration space registers
    uint16_t m_vendorID;
    uint16_t m_deviceID;
    uint16_t m_command;
    uint16_t m_status;
    uint8_t m_revisionID;
    uint8_t m_classcode[3];
    uint8_t m_headerType;
    uint8_t m_interruptLine;
    uint8_t m_interruptPin;
    
    // PCI base address registers (BARs)
    uint32_t m_bar0; // I/O base address
    uint32_t m_bar1; // Memory base address
    
    // I/O base address range
    uint16_t m_ioBase;
    uint16_t m_ioSize;
    
    // Memory-mapped I/O base address range
    uint32_t m_memBase;
    uint32_t m_memSize;
};

} // namespace x86emu