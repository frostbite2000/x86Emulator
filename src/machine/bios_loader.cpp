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

#include "machine/bios_loader.h"
#include "logger.h"
#include <fstream>
#include <filesystem>
#include <cstring>

namespace x86emu {

bool BiosLoader::loadBios(const std::shared_ptr<MachineConfig>& machine, MemoryManager* memory)
{
    if (!machine) {
        Logger::GetInstance()->error("Cannot load BIOS: Invalid machine configuration");
        return false;
    }
    
    if (!memory) {
        Logger::GetInstance()->error("Cannot load BIOS: Invalid memory manager");
        return false;
    }
    
    const auto& biosInfo = machine->getBiosInfo();
    
    if (m_usePlaceholderBios) {
        Logger::GetInstance()->info("Using placeholder BIOS for %s", machine->getName().c_str());
        return generatePlaceholderBios(memory, biosInfo.address, biosInfo.size);
    } else {
        Logger::GetInstance()->info("Loading BIOS %s for %s", biosInfo.filename.c_str(), machine->getName().c_str());
        return loadBiosFromFile(biosInfo.filename, memory, biosInfo.address, biosInfo.size);
    }
}

bool BiosLoader::loadBiosFromFile(const std::string& filename, MemoryManager* memory, uint32_t address, uint32_t expectedSize)
{
    std::string biosPath = createBiosPath(filename);
    
    // Check if file exists
    if (!std::filesystem::exists(biosPath)) {
        Logger::GetInstance()->error("BIOS file not found: %s", biosPath.c_str());
        return false;
    }
    
    // Get file size
    std::ifstream file(biosPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        Logger::GetInstance()->error("Failed to open BIOS file: %s", biosPath.c_str());
        return false;
    }
    
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Check file size
    if (fileSize != expectedSize) {
        Logger::GetInstance()->warn("BIOS file size mismatch: expected %u bytes, got %u bytes", 
                              expectedSize, static_cast<uint32_t>(fileSize));
    }
    
    // Read file content
    std::vector<uint8_t> buffer(fileSize);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        Logger::GetInstance()->error("Failed to read BIOS file: %s", biosPath.c_str());
        return false;
    }
    
    // Map BIOS to memory
    if (!memory->mapMemory(address, address + fileSize - 1, buffer.data(), MemoryPermissions::Read)) {
        Logger::GetInstance()->error("Failed to map BIOS to memory at address 0x%08X", address);
        return false;
    }
    
    // Verify BIOS signature (0x55, 0xAA at the end of the ROM)
    if (buffer[fileSize - 2] != 0x55 || buffer[fileSize - 1] != 0xAA) {
        Logger::GetInstance()->warn("BIOS file does not have valid signature (0x55, 0xAA)");
    }
    
    Logger::GetInstance()->info("BIOS loaded successfully: %s (%u bytes) at address 0x%08X", 
                          filename.c_str(), static_cast<uint32_t>(fileSize), address);
    
    return true;
}

bool BiosLoader::generatePlaceholderBios(MemoryManager* memory, uint32_t address, uint32_t size)
{
    // Create a buffer for the placeholder BIOS
    std::vector<uint8_t> buffer(size, 0xFF);  // Fill with 0xFF (unprogrammed flash)
    
    // Initialize the placeholder BIOS
    initPlaceholderBios(buffer.data(), size);
    
    // Map BIOS to memory
    if (!memory->mapMemory(address, address + size - 1, buffer.data(), MemoryPermissions::Read)) {
        Logger::GetInstance()->error("Failed to map placeholder BIOS to memory at address 0x%08X", address);
        return false;
    }
    
    Logger::GetInstance()->info("Placeholder BIOS generated successfully: %u bytes at address 0x%08X", 
                          size, address);
    
    return true;
}

std::string BiosLoader::createBiosPath(const std::string& filename) const
{
    std::filesystem::path biosDir(m_biosDirectory);
    std::filesystem::path biosFile(filename);
    return (biosDir / biosFile).string();
}

void BiosLoader::initPlaceholderBios(uint8_t* data, uint32_t size)
{
    if (!data || size < 0x10000) {
        return;  // BIOS must be at least 64KB
    }
    
    // Set BIOS signature at the end of the ROM
    data[size - 2] = 0x55;
    data[size - 1] = 0xAA;
    
    // F000:FFF0 - Reset vector (16 bytes from end)
    uint32_t resetOffset = size - 0x10;
    
    // Simple far jump to BIOS entry point
    // EA 00 00 00 FC - JMP FAR 0xFC00:0x0000
    data[resetOffset + 0] = 0xEA;  // JMP FAR
    data[resetOffset + 1] = 0x00;  // Offset low
    data[resetOffset + 2] = 0x00;  // Offset high
    data[resetOffset + 3] = 0x00;  // Segment low
    data[resetOffset + 4] = 0xFC;  // Segment high
    
    // BIOS entry point at FC00:0000
    uint32_t entryOffset = size - 0x4000;  // 16KB from end
    
    // Simple initialization code
    // Very basic initialization code that would just halt
    uint8_t initCode[] = {
        0xFA,                   // CLI - Clear interrupts
        0x31, 0xC0,             // XOR AX, AX
        0x8E, 0xD8,             // MOV DS, AX
        0x8E, 0xC0,             // MOV ES, AX
        0x8E, 0xD0,             // MOV SS, AX
        0xBC, 0x00, 0x7C,       // MOV SP, 0x7C00
        0xFB,                   // STI - Enable interrupts
        0xB0, 0x30,             // MOV AL, '0'
        0xB4, 0x0E,             // MOV AH, 0x0E
        0xCD, 0x10,             // INT 0x10 - Write character
        0xF4,                   // HLT - Halt
        0xEB, 0xFD,             // JMP $ - Infinite loop
    };
    
    // Copy initialization code to entry point
    std::memcpy(data + entryOffset, initCode, sizeof(initCode));
    
    // Add a simple BIOS data area
    uint32_t biosDataOffset = size - 0x3000;  // 12KB from end
    
    // BIOS identifier string
    const char* biosId = "x86Emulator Placeholder BIOS (Built: " __DATE__ " " __TIME__ ")";
    std::strcpy(reinterpret_cast<char*>(data + biosDataOffset), biosId);
}

} // namespace x86emu