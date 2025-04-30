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

#ifndef X86EMULATOR_MACHINE_CONFIG_H
#define X86EMULATOR_MACHINE_CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

namespace x86emu {

/**
 * @brief Machine configuration class
 * 
 * This class represents a specific machine configuration, including
 * hardware details and BIOS/ROM information.
 */
class MachineConfig {
public:
    /**
     * @brief Chipset type
     */
    enum class ChipsetType {
        SIS_630,
        SIS_730,
        INTEL_440BX,
        UNKNOWN
    };
    
    /**
     * @brief Machine form factor
     */
    enum class FormFactor {
        ATX,
        BABY_AT,
        MICRO_ATX,
        MINI_ITX,
        UNKNOWN
    };

    /**
     * @brief Machine hardware specification
     */
    struct HardwareSpec {
        ChipsetType chipset;
        FormFactor formFactor;
        int maxRamMB;
        bool hasOnboardAudio;
        bool hasOnboardVideo;
        bool hasOnboardLan;
        int numIdeChannels;
        bool hasFloppyController;
        int numSerialPorts;
        int numParallelPorts;
        int numUsbPorts;
        bool hasPs2Ports;
    };

    /**
     * @brief BIOS information
     */
    struct BiosInfo {
        std::string manufacturer;
        std::string version;
        std::string date;
        std::string filename;
        uint32_t size;
        uint32_t address;
    };

    /**
     * @brief Construct a new Machine Config
     * 
     * @param name Machine name
     * @param description Machine description
     */
    MachineConfig(const std::string& name, const std::string& description);
    
    /**
     * @brief Get machine name
     * 
     * @return Machine name
     */
    std::string getName() const { return m_name; }
    
    /**
     * @brief Get machine description
     * 
     * @return Machine description
     */
    std::string getDescription() const { return m_description; }
    
    /**
     * @brief Set hardware specification
     * 
     * @param spec Hardware specification
     */
    void setHardwareSpec(const HardwareSpec& spec) { m_hardwareSpec = spec; }
    
    /**
     * @brief Get hardware specification
     * 
     * @return Hardware specification
     */
    const HardwareSpec& getHardwareSpec() const { return m_hardwareSpec; }
    
    /**
     * @brief Set BIOS information
     * 
     * @param bios BIOS information
     */
    void setBiosInfo(const BiosInfo& bios) { m_biosInfo = bios; }
    
    /**
     * @brief Get BIOS information
     * 
     * @return BIOS information
     */
    const BiosInfo& getBiosInfo() const { return m_biosInfo; }
    
    /**
     * @brief Get BIOS filename
     * 
     * @return BIOS filename
     */
    std::string getBiosFilename() const { return m_biosInfo.filename; }
    
    /**
     * @brief Get chipset name
     * 
     * @return Chipset name
     */
    std::string getChipsetName() const;
    
    /**
     * @brief Get form factor name
     * 
     * @return Form factor name
     */
    std::string getFormFactorName() const;
    
    /**
     * @brief Set default configuration options
     * 
     * @param options Default configuration options
     */
    void setDefaultOptions(const std::map<std::string, std::string>& options) { m_defaultOptions = options; }
    
    /**
     * @brief Get default configuration options
     * 
     * @return Default configuration options
     */
    const std::map<std::string, std::string>& getDefaultOptions() const { return m_defaultOptions; }

private:
    std::string m_name;
    std::string m_description;
    HardwareSpec m_hardwareSpec;
    BiosInfo m_biosInfo;
    std::map<std::string, std::string> m_defaultOptions;
};

/**
 * @brief Machine configuration manager
 * 
 * This class manages available machine configurations.
 */
class MachineManager {
public:
    /**
     * @brief Get singleton instance
     * 
     * @return MachineManager instance
     */
    static MachineManager& getInstance();
    
    /**
     * @brief Initialize machine configurations
     */
    void initialize();
    
    /**
     * @brief Add a machine configuration
     * 
     * @param machine Machine configuration
     */
    void addMachine(std::shared_ptr<MachineConfig> machine);
    
    /**
     * @brief Get all available machines
     * 
     * @return Vector of available machine configurations
     */
    std::vector<std::shared_ptr<MachineConfig>> getAvailableMachines() const;
    
    /**
     * @brief Get machine by name
     * 
     * @param name Machine name
     * @return Machine configuration, or nullptr if not found
     */
    std::shared_ptr<MachineConfig> getMachineByName(const std::string& name) const;
    
    /**
     * @brief Create Zida V630E configuration
     * 
     * @return Machine configuration
     */
    static std::shared_ptr<MachineConfig> createZidaV630E();
    
    /**
     * @brief Create Shuttle MS11 configuration
     * 
     * @return Machine configuration
     */
    static std::shared_ptr<MachineConfig> createShuttleMS11();

private:
    MachineManager() = default;
    ~MachineManager() = default;

    // Prevent copying
    MachineManager(const MachineManager&) = delete;
    MachineManager& operator=(const MachineManager&) = delete;
    
    std::vector<std::shared_ptr<MachineConfig>> m_machines;
};

} // namespace x86emu

#endif // X86EMULATOR_MACHINE_CONFIG_H