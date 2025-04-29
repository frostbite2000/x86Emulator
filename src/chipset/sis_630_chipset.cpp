#include "sis_630_chipset.h"
#include <algorithm>
#include <cassert>

// Include MAME SiS 630 headers
// Note: Paths may need adjustment based on MAME's directory structure in your project
#include "mame/src/devices/machine/sis630.h"
#include "mame/src/emu/emumem.h"

namespace x86emu {

SiS630Chipset::SiS630Chipset(const std::string& name)
    : ChipsetDevice(name, "SiS 630/730 Chipset", ChipsetType::UNIFIED)
    , m_integratedGraphicsEnabled(true)
    , m_integratedGraphicsMemory(32) // Default to 32MB
{
    InitializeSupportedFeatures();
    InitializeDefaultOptions();
}

SiS630Chipset::~SiS630Chipset()
{
    // MAME devices will be cleaned up by their unique_ptr destructors
}

bool SiS630Chipset::Initialize()
{
    if (IsInitialized()) {
        return true;
    }
    
    try {
        // Create the MAME SiS 630 device
        m_device = std::make_unique<mame::sis630_device>(0);
        if (!m_device) {
            return false;
        }
        
        // Create the host device
        m_hostDevice = std::make_unique<mame::sis630_host_device>(0);
        if (!m_hostDevice) {
            m_device.reset();
            return false;
        }
        
        // Initialize the devices
        m_device->device_start();
        m_hostDevice->device_start();
        
        SetInitialized(true);
        return true;
    }
    catch (const std::exception& ex) {
        // Log the exception
        return false;
    }
}

void SiS630Chipset::Reset()
{
    if (IsInitialized()) {
        // Reset the MAME devices
        if (m_device) {
            m_device->device_reset();
        }
        
        if (m_hostDevice) {
            m_hostDevice->device_reset();
        }
    }
}

void SiS630Chipset::Update(uint32_t deltaTime)
{
    // Nothing to do here as the chipset is updated through I/O operations
}

std::vector<BusType> SiS630Chipset::GetSupportedBusTypes() const
{
    return { BusType::PCI, BusType::AGP, BusType::ISA };
}

uint32_t SiS630Chipset::GetMaxMemoryMB() const
{
    return 1024; // SiS 630 supports up to 1GB of system RAM
}

bool SiS630Chipset::SupportsFeature(const std::string& featureName) const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    auto it = m_supportedFeatures.find(featureName);
    return it != m_supportedFeatures.end() && it->second;
}

std::string SiS630Chipset::GetVersion() const
{
    return "SiS 630/730 (rev 31)"; // Typical revision
}

bool SiS630Chipset::SetOption(const std::string& option, const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Check if the option is valid
    if (m_options.find(option) == m_options.end()) {
        return false;
    }
    
    // Set the option value
    m_options[option] = value;
    
    // Apply the option if needed
    if (option == "integrated_graphics") {
        m_integratedGraphicsEnabled = (value == "enabled");
    }
    else if (option == "integrated_graphics_memory") {
        try {
            m_integratedGraphicsMemory = std::stoul(value);
            // Clamp to valid values (8MB to 64MB)
            m_integratedGraphicsMemory = std::max(8u, std::min(64u, m_integratedGraphicsMemory));
        }
        catch (...) {
            return false;
        }
    }
    
    return true;
}

std::string SiS630Chipset::GetOption(const std::string& option) const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    auto it = m_options.find(option);
    if (it != m_options.end()) {
        return it->second;
    }
    
    return "";
}

std::string SiS630Chipset::GetIntegratedGraphicsID() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_integratedGraphicsEnabled ? "sis630_vga" : "";
}

bool SiS630Chipset::EnableIntegratedGraphics(bool enabled)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    m_integratedGraphicsEnabled = enabled;
    m_options["integrated_graphics"] = enabled ? "enabled" : "disabled";
    
    // In a real implementation, we would also need to update the MAME device
    // This is simplified and would need to be adapted to the actual API
    
    return true;
}

bool SiS630Chipset::IsIntegratedGraphicsEnabled() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_integratedGraphicsEnabled;
}

bool SiS630Chipset::SetIntegratedGraphicsMemory(uint32_t sizeMB)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Validate the memory size (SiS 630 supports 8MB to 64MB for graphics)
    if (sizeMB < 8 || sizeMB > 64) {
        return false;
    }
    
    m_integratedGraphicsMemory = sizeMB;
    m_options["integrated_graphics_memory"] = std::to_string(sizeMB);
    
    // In a real implementation, we would also need to update the MAME device
    // This is simplified and would need to be adapted to the actual API
    
    return true;
}

uint32_t SiS630Chipset::GetIntegratedGraphicsMemory() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_integratedGraphicsMemory;
}

void SiS630Chipset::InitializeSupportedFeatures()
{
    // SiS 630 features
    m_supportedFeatures["integrated_graphics"] = true;
    m_supportedFeatures["usb"] = true;
    m_supportedFeatures["ac97"] = true;
    m_supportedFeatures["ide"] = true;
    m_supportedFeatures["agp"] = true;
    m_supportedFeatures["sdram"] = true;
    m_supportedFeatures["power_management"] = true;
    m_supportedFeatures["pci"] = true;
}

void SiS630Chipset::InitializeDefaultOptions()
{
    // Default options for SiS 630
    m_options["integrated_graphics"] = "enabled";
    m_options["integrated_graphics_memory"] = "32";
    m_options["usb_controller"] = "enabled";
    m_options["ac97_audio"] = "enabled";
    m_options["ide_controller"] = "enabled";
    m_options["power_management"] = "enabled";
    m_options["memory_speed"] = "133MHz";
}

} // namespace x86emu