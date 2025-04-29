#pragma once

#include "x86emulator/chipset_device.h"
#include <mutex>
#include <unordered_map>
#include <vector>

// Forward declarations for MAME types
namespace mame {
class sis630_device;
class sis630_host_device;
struct sis_agp_aperture;
}

namespace x86emu {

/**
 * @brief SiS 630 chipset implementation using MAME.
 * 
 * This class adapts MAME's SiS 630 implementation to the x86Emulator
 * ChipsetDevice interface.
 */
class SiS630Chipset : public ChipsetDevice {
public:
    /**
     * Create a SiS 630 chipset.
     * @param name Unique device name
     */
    explicit SiS630Chipset(const std::string& name);
    
    /**
     * Destructor.
     */
    ~SiS630Chipset() override;

    // Device interface implementation
    bool Initialize() override;
    void Reset() override;
    void Update(uint32_t deltaTime) override;
    
    // ChipsetDevice interface implementation
    std::vector<BusType> GetSupportedBusTypes() const override;
    uint32_t GetMaxMemoryMB() const override;
    bool SupportsFeature(const std::string& featureName) const override;
    std::string GetVersion() const override;
    bool SetOption(const std::string& option, const std::string& value) override;
    std::string GetOption(const std::string& option) const override;
    
    /**
     * Get the integrated graphics device ID.
     * @return ID of the integrated graphics device
     */
    std::string GetIntegratedGraphicsID() const;
    
    /**
     * Enable or disable the integrated graphics.
     * @param enabled true to enable, false to disable
     * @return true if the operation was successful
     */
    bool EnableIntegratedGraphics(bool enabled);
    
    /**
     * Check if integrated graphics is enabled.
     * @return true if integrated graphics is enabled
     */
    bool IsIntegratedGraphicsEnabled() const;
    
    /**
     * Set the amount of memory allocated to integrated graphics.
     * @param sizeMB Amount of memory in MB
     * @return true if the operation was successful
     */
    bool SetIntegratedGraphicsMemory(uint32_t sizeMB);
    
    /**
     * Get the amount of memory allocated to integrated graphics.
     * @return Amount of memory in MB
     */
    uint32_t GetIntegratedGraphicsMemory() const;
    
private:
    // MAME SiS 630 device
    std::unique_ptr<mame::sis630_device> m_device;
    std::unique_ptr<mame::sis630_host_device> m_hostDevice;
    
    // Supported features
    std::unordered_map<std::string, bool> m_supportedFeatures;
    
    // Configuration options
    std::unordered_map<std::string, std::string> m_options;
    
    // Integrated graphics state
    bool m_integratedGraphicsEnabled;
    uint32_t m_integratedGraphicsMemory; // in MB
    
    // Access synchronization
    mutable std::mutex m_accessMutex;
    
    // Initialize the supported features list
    void InitializeSupportedFeatures();
    
    // Initialize default options
    void InitializeDefaultOptions();
};

} // namespace x86emu