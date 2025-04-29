#pragma once

#include <memory>
#include <string>

namespace x86emu {

class CPU;
class Memory;
class IO;

/**
 * @brief CPU backend implementation options.
 */
enum class CPUBackend {
    BOX86,  // Use 86Box CPU implementation
    MAME    // Use MAME i386 CPU implementation
    // Remove WinUAE backend
    // WINUAE  // Use WinUAE CPU implementation
};

/**
 * @brief Machine types.
 */
enum class MachineType {
    GENERIC_PENTIUM,     // Generic Pentium system with Intel 440BX
    SIS_630_SYSTEM,      // SiS 630-based system
    VIA_EPIA,            // VIA EPIA system
    CUSTOM               // Custom configuration
};

/**
 * @brief Main emulation controller.
 * 
 * This class orchestrates the emulation process, connecting
 * CPU, memory, I/O, and device emulation components.
 */
class Emulation {
public:
    /**
     * Create an emulation instance.
     * @return Unique pointer to emulation instance
     */
    static std::unique_ptr<Emulation> Create();
    
    /**
     * Destructor.
     */
    virtual ~Emulation() = default;
    
    /**
     * Initialize the emulation.
     * @return true if initialization was successful
     */
    virtual bool Initialize() = 0;
    
    /**
     * Shut down the emulation.
     */
    virtual void Shutdown() = 0;
    
    /**
     * Reset the emulation.
     */
    virtual void Reset() = 0;
    
    /**
     * Run a single frame of emulation.
     * @return true if the frame was run successfully
     */
    virtual bool RunFrame() = 0;
    
    /**
     * Pause or resume emulation.
     * @param pause true to pause, false to resume
     */
    virtual void Pause(bool pause) = 0;
    
    /**
     * Check if emulation is paused.
     * @return true if paused
     */
    virtual bool IsPaused() const = 0;
    
    /**
     * Check if emulation is initialized.
     * @return true if initialized
     */
    virtual bool IsInitialized() const = 0;
    
    /**
     * Load emulation configuration from file.
     * @param filename Configuration file path
     * @return true if configuration was loaded successfully
     */
    virtual bool LoadConfiguration(const std::string& filename) = 0;
    
    /**
     * Save emulation configuration to file.
     * @param filename Configuration file path
     * @return true if configuration was saved successfully
     */
    virtual bool SaveConfiguration(const std::string& filename) = 0;
    
    /**
     * Get the CPU interface.
     * @return Pointer to CPU interface
     */
    virtual CPU* GetCPU() const = 0;
    
    /**
     * Get the memory interface.
     * @return Pointer to memory interface
     */
    virtual Memory* GetMemory() const = 0;
    
    /**
     * Get the I/O interface.
     * @return Pointer to I/O interface
     */
    virtual IO* GetIO() const = 0;
    
    /**
     * Set the CPU backend to use.
     * Must be called before Initialize().
     * @param backend The CPU backend to use
     */
    virtual void SetCPUBackend(CPUBackend backend) = 0;
    
    /**
     * Get the current CPU backend.
     * @return The CPU backend in use
     */
    virtual CPUBackend GetCPUBackend() const = 0;
    
    /**
     * Set the machine type to emulate.
     * @param type Machine type
     * @return true if the machine type was set successfully
     */
    virtual bool SetMachineType(MachineType type) = 0;
    
    /**
     * Get the current machine type.
     * @return Current machine type
     */
    virtual MachineType GetMachineType() const = 0;
};

} // namespace x86emu