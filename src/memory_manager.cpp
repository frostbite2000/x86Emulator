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

#include "memory_manager.h"
#include "logger.h"

#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <filesystem>

// Constants
constexpr uint32_t KB = 1024;
constexpr uint32_t MB = 1024 * KB;
constexpr uint32_t DEFAULT_MEMORY_SIZE = 640 * KB;
constexpr uint32_t BIOS_BASE_ADDRESS = 0xF0000;
constexpr uint32_t BIOS_SIZE = 64 * KB;
constexpr uint32_t EXTENDED_MEMORY_BASE = 1 * MB;
constexpr uint32_t MAX_MEMORY_SIZE = 4 * GB;

MemoryManager::MemoryManager()
    : m_nextCallbackId(1),
      m_totalSize(0),
      m_conventionalSize(0),
      m_extendedSize(0)
{
}

MemoryManager::~MemoryManager()
{
    // Release all memory regions
    m_regions.clear();
}

bool MemoryManager::initialize(int memorySize)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Calculate memory sizes
        m_conventionalSize = std::min(static_cast<uint32_t>(memorySize * KB), 640 * KB);
        
        // Extended memory (above 1 MB)
        if (memorySize > 1024) {
            m_extendedSize = static_cast<uint32_t>((memorySize - 1024) * KB);
        } else {
            m_extendedSize = 0;
        }
        
        // Total memory size (including MMIO and ROM regions)
        m_totalSize = 16 * MB; // Support up to 16 MB addressable memory
        
        // Allocate memory
        m_memory.resize(m_totalSize, 0);
        
        // Clear memory
        reset();
        
        // Register conventional memory region (0 - 640 KB)
        registerMemoryRegion(0, m_conventionalSize, RegionType::RAM, "Conventional Memory", true, true, true);
        
        // Register extended memory region (1 MB - 16 MB)
        if (m_extendedSize > 0) {
            registerMemoryRegion(EXTENDED_MEMORY_BASE, m_extendedSize, RegionType::RAM, "Extended Memory", true, true, true);
        }
        
        // Register BIOS ROM region
        registerMemoryRegion(BIOS_BASE_ADDRESS, BIOS_SIZE, RegionType::ROM, "BIOS ROM", true, false, true);
        
        Logger::GetInstance()->info("Memory initialized: %d KB conventional, %d KB extended", 
                                  m_conventionalSize / KB, m_extendedSize / KB);
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Memory initialization failed: %s", ex.what());
        return false;
    }
}

uint8_t MemoryManager::readByte(uint32_t address) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is valid
    if (address >= m_totalSize) {
        // Invalid address - return 0xFF
        Logger::GetInstance()->warn("Invalid memory read at 0x%08X", address);
        return 0xFF;
    }
    
    // Check memory access
    if (!checkAccess(address, 1, AccessType::READ)) {
        // Read violation - return 0xFF
        Logger::GetInstance()->warn("Memory read violation at 0x%08X", address);
        return 0xFF;
    }
    
    // Notify callbacks
    const_cast<MemoryManager*>(this)->notifyCallbacks(address, 1, AccessType::READ);
    
    // Return memory value
    return m_memory[address];
}

uint16_t MemoryManager::readWord(uint32_t address) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is valid
    if (address + 1 >= m_totalSize) {
        // Invalid address - return 0xFFFF
        Logger::GetInstance()->warn("Invalid memory read at 0x%08X", address);
        return 0xFFFF;
    }
    
    // Check memory access
    if (!checkAccess(address, 2, AccessType::READ)) {
        // Read violation - return 0xFFFF
        Logger::GetInstance()->warn("Memory read violation at 0x%08X", address);
        return 0xFFFF;
    }
    
    // Notify callbacks
    const_cast<MemoryManager*>(this)->notifyCallbacks(address, 2, AccessType::READ);
    
    // Return memory value
    return *reinterpret_cast<const uint16_t*>(&m_memory[address]);
}

uint32_t MemoryManager::readDword(uint32_t address) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is valid
    if (address + 3 >= m_totalSize) {
        // Invalid address - return 0xFFFFFFFF
        Logger::GetInstance()->warn("Invalid memory read at 0x%08X", address);
        return 0xFFFFFFFF;
    }
    
    // Check memory access
    if (!checkAccess(address, 4, AccessType::READ)) {
        // Read violation - return 0xFFFFFFFF
        Logger::GetInstance()->warn("Memory read violation at 0x%08X", address);
        return 0xFFFFFFFF;
    }
    
    // Notify callbacks
    const_cast<MemoryManager*>(this)->notifyCallbacks(address, 4, AccessType::READ);
    
    // Return memory value
    return *reinterpret_cast<const uint32_t*>(&m_memory[address]);
}

void MemoryManager::writeByte(uint32_t address, uint8_t value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is valid
    if (address >= m_totalSize) {
        // Invalid address
        Logger::GetInstance()->warn("Invalid memory write at 0x%08X", address);
        return;
    }
    
    // Check memory access
    if (!checkAccess(address, 1, AccessType::WRITE)) {
        // Write violation
        Logger::GetInstance()->warn("Memory write violation at 0x%08X", address);
        return;
    }
    
    // Notify callbacks
    notifyCallbacks(address, 1, AccessType::WRITE);
    
    // Write memory value
    m_memory[address] = value;
}

void MemoryManager::writeWord(uint32_t address, uint16_t value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is valid
    if (address + 1 >= m_totalSize) {
        // Invalid address
        Logger::GetInstance()->warn("Invalid memory write at 0x%08X", address);
        return;
    }
    
    // Check memory access
    if (!checkAccess(address, 2, AccessType::WRITE)) {
        // Write violation
        Logger::GetInstance()->warn("Memory write violation at 0x%08X", address);
        return;
    }
    
    // Notify callbacks
    notifyCallbacks(address, 2, AccessType::WRITE);
    
    // Write memory value
    *reinterpret_cast<uint16_t*>(&m_memory[address]) = value;
}

void MemoryManager::writeDword(uint32_t address, uint32_t value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is valid
    if (address + 3 >= m_totalSize) {
        // Invalid address
        Logger::GetInstance()->warn("Invalid memory write at 0x%08X", address);
        return;
    }
    
    // Check memory access
    if (!checkAccess(address, 4, AccessType::WRITE)) {
        // Write violation
        Logger::GetInstance()->warn("Memory write violation at 0x%08X", address);
        return;
    }
    
    // Notify callbacks
    notifyCallbacks(address, 4, AccessType::WRITE);
    
    // Write memory value
    *reinterpret_cast<uint32_t*>(&m_memory[address]) = value;
}

bool MemoryManager::registerMemoryRegion(uint32_t start, uint32_t size, RegionType type, const std::string& name, bool readable, bool writable, bool executable)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check for overlapping regions
    for (const auto& region : m_regions) {
        if ((start < region.start + region.size) && (start + size > region.start)) {
            Logger::GetInstance()->warn("Memory region overlap: 0x%08X-0x%08X with %s", start, start + size - 1, region.name.c_str());
            return false;
        }
    }
    
    // Check if region is within memory bounds
    if (start + size > m_totalSize) {
        Logger::GetInstance()->warn("Memory region out of bounds: 0x%08X-0x%08X", start, start + size - 1);
        return false;
    }
    
    // Create new region
    MemoryRegion region;
    region.start = start;
    region.size = size;
    region.type = type;
    region.name = name;
    region.readable = readable;
    region.writable = writable;
    region.executable = executable;
    region.data = &m_memory[start];
    
    // Add to regions list
    m_regions.push_back(region);
    
    Logger::GetInstance()->info("Registered memory region: %s at 0x%08X-0x%08X", name.c_str(), start, start + size - 1);
    return true;
}

bool MemoryManager::unregisterMemoryRegion(uint32_t start, uint32_t size)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find region
    for (auto it = m_regions.begin(); it != m_regions.end(); ++it) {
        if (it->start == start && it->size == size) {
            // Remove region
            m_regions.erase(it);
            
            Logger::GetInstance()->info("Unregistered memory region at 0x%08X-0x%08X", start, start + size - 1);
            return true;
        }
    }
    
    Logger::GetInstance()->warn("Memory region not found: 0x%08X-0x%08X", start, start + size - 1);
    return false;
}

const MemoryManager::MemoryRegion* MemoryManager::getMemoryRegion(uint32_t address) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find region containing address
    for (const auto& region : m_regions) {
        if (address >= region.start && address < region.start + region.size) {
            return &region;
        }
    }
    
    return nullptr;
}

bool MemoryManager::isRAM(uint32_t address) const
{
    const MemoryRegion* region = getMemoryRegion(address);
    return region && region->type == RegionType::RAM;
}

bool MemoryManager::isROM(uint32_t address) const
{
    const MemoryRegion* region = getMemoryRegion(address);
    return region && region->type == RegionType::ROM;
}

bool MemoryManager::isMMIO(uint32_t address) const
{
    const MemoryRegion* region = getMemoryRegion(address);
    return region && region->type == RegionType::MMIO;
}

uint8_t* MemoryManager::getPointer(uint32_t address)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is valid
    if (address >= m_totalSize) {
        return nullptr;
    }
    
    // Find region containing address
    const MemoryRegion* region = getMemoryRegion(address);
    if (!region) {
        return nullptr;
    }
    
    // Return pointer
    return &m_memory[address];
}

const uint8_t* MemoryManager::getPointer(uint32_t address) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is valid
    if (address >= m_totalSize) {
        return nullptr;
    }
    
    // Find region containing address
    const MemoryRegion* region = getMemoryRegion(address);
    if (!region) {
        return nullptr;
    }
    
    // Return pointer
    return &m_memory[address];
}

uint32_t MemoryManager::registerCallback(uint32_t address, uint32_t size, AccessType type, MemoryCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Create new callback
    CallbackInfo info;
    info.id = m_nextCallbackId++;
    info.address = address;
    info.size = size;
    info.type = type;
    info.callback = callback;
    
    // Add to callbacks list
    m_callbacks.push_back(info);
    
    return info.id;
}

bool MemoryManager::unregisterCallback(uint32_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find callback
    for (auto it = m_callbacks.begin(); it != m_callbacks.end(); ++it) {
        if (it->id == id) {
            // Remove callback
            m_callbacks.erase(it);
            return true;
        }
    }
    
    return false;
}

bool MemoryManager::loadBIOS(const std::string& biosPath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Open BIOS file
        std::ifstream file(biosPath, std::ios::binary);
        if (!file) {
            Logger::GetInstance()->error("Failed to open BIOS file: %s", biosPath.c_str());
            return false;
        }
        
        // Check file size
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        if (size > BIOS_SIZE) {
            Logger::GetInstance()->warn("BIOS file too large, truncating to %d KB", BIOS_SIZE / KB);
            size = BIOS_SIZE;
        }
        
        // Read BIOS into memory
        file.read(reinterpret_cast<char*>(&m_memory[BIOS_BASE_ADDRESS]), size);
        
        Logger::GetInstance()->info("Loaded BIOS from %s (%d bytes)", biosPath.c_str(), size);
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Failed to load BIOS: %s", ex.what());
        return false;
    }
}

void MemoryManager::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clear memory (skip ROM regions)
    for (const auto& region : m_regions) {
        if (region.type == RegionType::RAM) {
            // Clear RAM region
            std::memset(region.data, 0, region.size);
        }
    }
    
    Logger::GetInstance()->info("Memory reset");
}

bool MemoryManager::dumpMemory(const std::string& path, uint32_t start, uint32_t size) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Check if address range is valid
        if (start + size > m_totalSize) {
            Logger::GetInstance()->error("Invalid memory range for dump: 0x%08X-0x%08X", start, start + size - 1);
            return false;
        }
        
        // Open dump file
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            Logger::GetInstance()->error("Failed to open memory dump file: %s", path.c_str());
            return false;
        }
        
        // Write memory to file
        file.write(reinterpret_cast<const char*>(&m_memory[start]), size);
        
        Logger::GetInstance()->info("Memory dumped to %s (0x%08X-0x%08X, %d bytes)", path.c_str(), start, start + size - 1, size);
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Failed to dump memory: %s", ex.what());
        return false;
    }
}

void MemoryManager::notifyCallbacks(uint32_t address, uint32_t size, AccessType type)
{
    // Check all callbacks
    for (const auto& callback : m_callbacks) {
        // Check if callback applies to this access
        if (callback.type == type && 
            address >= callback.address && 
            address + size <= callback.address + callback.size) {
            // Call the callback
            callback.callback(address, size);
        }
    }
}

bool MemoryManager::checkAccess(uint32_t address, uint32_t size, AccessType type) const
{
    // Check all regions that overlap with the access
    for (uint32_t offset = 0; offset < size; ++offset) {
        uint32_t currentAddress = address + offset;
        const MemoryRegion* region = getMemoryRegion(currentAddress);
        
        if (!region) {
            // Address not in any region
            return false;
        }
        
        // Check permissions
        switch (type) {
            case AccessType::READ:
                if (!region->readable) return false;
                break;
            case AccessType::WRITE:
                if (!region->writable) return false;
                break;
            case AccessType::EXECUTE:
                if (!region->executable) return false;
                break;
        }
    }
    
    return true;
}