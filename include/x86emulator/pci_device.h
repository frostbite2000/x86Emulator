#pragma once

#include "x86emulator/device.h"
#include <cstdint>
#include <string>

namespace x86emu {

/**
 * @brief Base class for PCI devices.
 * 
 * This class provides the interface for PCI device emulation.
 */
class PCIDevice : public Device {
public:
    /**
     * Constructor with device name and description.
     * @param name Unique identifier for the device
     * @param description Human-readable description
     */
    PCIDevice(const std::string& name, const std::string& description);
    
    /**
     * Destructor.
     */
    virtual ~PCIDevice() = default;
    
    /**
     * Read from the PCI configuration space.
     * @param reg Register offset
     * @param size Size of the read (1, 2, or 4 bytes)
     * @return Value read from the configuration space
     */
    virtual uint32_t ReadConfig(uint8_t reg, int size) = 0;
    
    /**
     * Write to the PCI configuration space.
     * @param reg Register offset
     * @param value Value to write
     * @param size Size of the write (1, 2, or 4 bytes)
     */
    virtual void WriteConfig(uint8_t reg, uint32_t value, int size) = 0;
    
    /**
     * Read from memory-mapped I/O space.
     * @param address Memory address
     * @param size Size of the read (1, 2, or 4 bytes)
     * @return Value read from memory
     */
    virtual uint32_t ReadMemory(uint32_t address, int size) = 0;
    
    /**
     * Write to memory-mapped I/O space.
     * @param address Memory address
     * @param value Value to write
     * @param size Size of the write (1, 2, or 4 bytes)
     */
    virtual void WriteMemory(uint32_t address, uint32_t value, int size) = 0;
    
    /**
     * Read from I/O port space.
     * @param address I/O port address
     * @param size Size of the read (1, 2, or 4 bytes)
     * @return Value read from I/O port
     */
    virtual uint32_t ReadIO(uint16_t address, int size) = 0;
    
    /**
     * Write to I/O port space.
     * @param address I/O port address
     * @param value Value to write
     * @param size Size of the write (1, 2, or 4 bytes)
     */
    virtual void WriteIO(uint16_t address, uint32_t value, int size) = 0;
    
    /**
     * Get the PCI bus number.
     * @return PCI bus number
     */
    uint8_t GetBus() const;
    
    /**
     * Get the PCI device number.
     * @return PCI device number
     */
    uint8_t GetDevice() const;
    
    /**
     * Get the PCI function number.
     * @return PCI function number
     */
    uint8_t GetFunction() const;
    
    /**
     * Set the PCI address (bus, device, function).
     * @param bus Bus number
     * @param device Device number
     * @param function Function number
     */
    void SetPCIAddress(uint8_t bus, uint8_t device, uint8_t function);

protected:
    uint8_t m_bus;      // PCI bus number
    uint8_t m_device;   // PCI device number
    uint8_t m_function; // PCI function number
};

} // namespace x86emu