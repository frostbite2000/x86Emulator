#include "x86emulator/pci_device.h"
#include <cassert>

namespace x86emu {

PCIDevice::PCIDevice(const std::string& name, const std::string& description)
    : Device(name, description)
    , m_bus(0)
    , m_device(0)
    , m_function(0)
{
}

uint8_t PCIDevice::GetBus() const
{
    return m_bus;
}

uint8_t PCIDevice::GetDevice() const
{
    return m_device;
}

uint8_t PCIDevice::GetFunction() const
{
    return m_function;
}

void PCIDevice::SetPCIAddress(uint8_t bus, uint8_t device, uint8_t function)
{
    assert(device < 32);   // Valid device numbers are 0-31
    assert(function < 8);  // Valid function numbers are 0-7
    
    m_bus = bus;
    m_device = device;
    m_function = function;
}

} // namespace x86emu