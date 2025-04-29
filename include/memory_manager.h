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

#ifndef X86EMULATOR_MEMORY_MANAGER_H
#define X86EMULATOR_MEMORY_MANAGER_H

#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <string>

/**
 * @brief Memory manager for the emulator
 * 
 * Handles memory allocation, access, and mapping for the emulated system.
 */
class MemoryManager {
public:
    /**
     * @brief Callback type for memory read/write notification
     */
    using MemoryCallback = std::function<void(uint32_t, uint32_t)>;
    
    /**
     * @brief Memory access type
     */
    enum class AccessType {
        READ,
        WRITE,
        EXECUTE
    };
    
    /**
     * @brief Memory region type
     */
    enum class RegionType {
        RAM,      // Regular RAM
        ROM,      // Read-only memory
        MMIO,     // Memory-mapped I/O
        VRAM,     // Video RAM
        SHADOW,   // Shadow RAM (e.g. for ROM shadowing)
        RESERVED  // Reserved/unused
    };
    
    /**
     * @brief Memory region descriptor
     */
    struct MemoryRegion {
        uint32_t start;      // Start address
        uint32_t size;       // Region size
        RegionType type;     // Region type
        bool readable;       // Can be read
        bool writable;       // Can be written
        bool executable;     // Can be executed
        std::string name;    // Region name
        uint8_t* data;       // Pointer to memory data
    };
    
    /**
     * @brief Construct a new Memory Manager
     */
    MemoryManager();
    
    /**
     * @brief Destroy the Memory Manager
     */
    ~MemoryManager();
    
    /**
     * @brief Initialize memory system
     * 
     * @param memorySize Size of conventional memory in KB
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize(int memorySize);
    
    /**
     * @brief Read a byte from memory
     * 
     * @param address Physical memory address
     * @return Byte value
     */
    uint8_t readByte(uint32_t address) const;
    
    /**
     * @brief Read a word (2 bytes) from memory
     * 
     * @param address Physical memory address
     * @return Word value
     */
    uint16_t readWord(uint32_t address) const;
    
    /**
     * @brief Read a double word (4 bytes) from memory
     * 
     * @param address Physical memory address
     * @return Double word value
     */
    uint32_t readDword(uint32_t address) const;
    
    /**
     * @brief Write a byte to memory
     * 
     * @param address Physical memory address
     * @param value Byte value
     */
    void writeByte(uint32_t address, uint8_t value);
    
    /**
     * @brief Write a word (2 bytes) to memory
     * 
     * @param address Physical memory address
     * @param value Word value
     */
    void writeWord(uint32_t address, uint16_t value);
    
    /**
     * @brief Write a double word (4 bytes) to memory
     * 
     * @param address Physical memory address
     * @param value Double word value
     */
    void writeDword(uint32_t address, uint32_t value);
    
    /**
     * @brief Register a memory region
     * 
     * @param start Start address
     * @param size Region size
     * @param type Region type
     * @param name Region name
     * @param readable Whether the region is readable
     * @param writable Whether the region is writable
     * @param executable Whether the region is executable
     * @return true if registration was successful
     * @return false if registration failed
     */
    bool registerMemoryRegion(uint32_t start, uint32_t size, RegionType type, const std::string& name, bool readable, bool writable, bool executable);
    
    /**
     * @brief Unregister a memory region
     * 
     * @param start Start address
     * @param size Region size
     * @return true if unregistration was successful
     * @return false if unregistration failed
     */
    bool unregisterMemoryRegion(uint32_t start, uint32_t size);
    
    /**
     * @brief Get information about a memory region
     * 
     * @param address Address within the region
     * @return Pointer to the memory region, or nullptr if not found
     */
    const MemoryRegion* getMemoryRegion(uint32_t address) const;
    
    /**
     * @brief Check if address is in RAM
     * 
     * @param address Memory address to check
     * @return true if address is in RAM
     * @return false if address is not in RAM
     */
    bool isRAM(uint32_t address) const;
    
    /**
     * @brief Check if address is in ROM
     * 
     * @param address Memory address to check
     * @return true if address is in ROM
     * @return false if address is not in ROM
     */
    bool isROM(uint32_t address) const;
    
    /**
     * @brief Check if address is in MMIO
     * 
     * @param address Memory address to check
     * @return true if address is in MMIO
     * @return false if address is not in MMIO
     */
    bool isMMIO(uint32_t address) const;
    
    /**
     * @brief Get direct pointer to memory
     * 
     * @param address Memory address
     * @return Pointer to memory, or nullptr if invalid
     */
    uint8_t* getPointer(uint32_t address);
    
    /**
     * @brief Get const direct pointer to memory
     * 
     * @param address Memory address
     * @return Const pointer to memory, or nullptr if invalid
     */
    const uint8_t* getPointer(uint32_t address) const;
    
    /**
     * @brief Register a callback for memory access
     * 
     * @param address Start address
     * @param size Region size
     * @param type Access type
     * @param callback Callback function
     * @return Registration ID, or 0 if registration failed
     */
    uint32_t registerCallback(uint32_t address, uint32_t size, AccessType type, MemoryCallback callback);
    
    /**
     * @brief Unregister a callback
     * 
     * @param id Registration ID
     * @return true if unregistration was successful
     * @return false if unregistration failed
     */
    bool unregisterCallback(uint32_t id);
    
    /**
     * @brief Load BIOS from file
     * 
     * @param biosPath Path to the BIOS file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadBIOS(const std::string& biosPath);
    
    /**
     * @brief Reset memory
     * 
     * Reset all RAM to 0
     */
    void reset();
    
    /**
     * @brief Get the total memory size
     * 
     * @return Total memory size in bytes
     */
    uint32_t getTotalSize() const { return m_totalSize; }
    
    /**
     * @brief Get the conventional memory size
     * 
     * @return Conventional memory size in bytes
     */
    uint32_t getConventionalSize() const { return m_conventionalSize; }
    
    /**
     * @brief Get the extended memory size
     * 
     * @return Extended memory size in bytes
     */
    uint32_t getExtendedSize() const { return m_extendedSize; }
    
    /**
     * @brief Dump memory to file
     * 
     * @param path File path
     * @param start Start address
     * @param size Size to dump
     * @return true if dump was successful
     * @return false if dump failed
     */
    bool dumpMemory(const std::string& path, uint32_t start, uint32_t size) const;

private:
    // Memory data
    std::vector<uint8_t> m_memory;
    
    // Memory regions
    std::vector<MemoryRegion> m_regions;
    
    // Memory callbacks
    struct CallbackInfo {
        uint32_t id;
        uint32_t address;
        uint32_t size;
        AccessType type;
        MemoryCallback callback;
    };
    
    std::vector<CallbackInfo> m_callbacks;
    uint32_t m_nextCallbackId;
    
    // Memory sizes
    uint32_t m_totalSize;
    uint32_t m_conventionalSize;
    uint32_t m_extendedSize;
    
    // Mutex for thread safety
    mutable std::mutex m_mutex;
    
    // Helper methods
    void notifyCallbacks(uint32_t address, uint32_t size, AccessType type);
    bool checkAccess(uint32_t address, uint32_t size, AccessType type) const;
};

#endif // X86EMULATOR_MEMORY_MANAGER_H