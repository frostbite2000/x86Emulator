/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2025 frostbite2000
 * Portions Copyright (C) 2011-2025 MAME development team (original SiS 630/730 implementation)
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

#include "devices/machine/sis630_host.h"
#include <algorithm>
#include <cstdarg>

namespace x86emu {

SiS630HostDevice::SiS630HostDevice(uint32_t ram_size)
    : PCIHostDevice(0x10390630, 0x01, 0x060000, 0x00)
    , m_ram_size(ram_size)
    , m_shadow_ram_ctrl(0)
    , m_smram(0)
    , m_vga_control(0)
    , m_dram_status(0)
    , m_pci_hole(0)
    , m_agp_mailbox{}
    , m_agp_priority_timer(0)
    , m_agp{false, false, 0}
{
    // Northbridge Host to PCI Bridge (multifunction device)
    setMultiFunction(true);
}

SiS630HostDevice::~SiS630HostDevice()
{
}

bool SiS630HostDevice::initialize()
{
    // Allocate RAM
    m_ram.resize(m_ram_size / 4, 0);
    
    // Initialize base PCI device
    if (!PCIHostDevice::initialize()) {
        return false;
    }
    
    Logger::GetInstance()->info("SiS 630 Host controller initialized with %d MB RAM", m_ram_size / (1024 * 1024));
    return true;
}

void SiS630HostDevice::reset()
{
    PCIHostDevice::reset();
    
    // Reset PCI configuration registers
    setCommand(0x0005);
    setStatus(0x0210);
    
    // Reset SiS-specific registers
    m_shadow_ram_ctrl = 0;
    m_smram = 0;
    m_vga_control = 0;
    m_dram_status = 0;
    m_pci_hole = 0;
    std::fill(m_agp_mailbox.begin(), m_agp_mailbox.end(), 0);
    m_agp_priority_timer = 0;
    
    // Reset AGP state
    m_agp.sba_enable = false;
    m_agp.enable = false;
    m_agp.data_rate = 0;
}

uint32_t SiS630HostDevice::configRead(int function, int reg, uint32_t mem_mask)
{
    // Only handle function 0 for now (host bridge)
    if (function != 0) {
        return 0xFFFFFFFF;
    }
    
    switch (reg) {
        // Header type override: report as bridge (0x80)
        case 0x0e:
            return 0x80;
            
        // Custom PCI capability pointer
        case 0x34:
            return readCapPtr();
            
        // SiS-specific registers
        case 0x63:
            return readDramStatus();
            
        case 0x6a:
            return readSmram();
            
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
            return readShadowRamCtrl();
            
        case 0x77:
            return readPCIHole();
            
        case 0x9c:
            return readVgaControl();
            
        case 0xa0:
        case 0xa1:
        case 0xa2:
        case 0xa3:
            return readAgpPriorityTimer();
        
        case 0xa4:
        case 0xa5:
        case 0xa6:
        case 0xa7:
        case 0xa8:
        case 0xa9:
        case 0xaa:
        case 0xab:
        case 0xac:
        case 0xad:
        case 0xae:
        case 0xaf:
            return readAgpMailbox(reg - 0xa4);
            
        case 0xc0:
        case 0xc1:
        case 0xc2:
        case 0xc3:
            return readAgpId();
            
        case 0xc4:
        case 0xc5:
        case 0xc6:
        case 0xc7:
            return readAgpStatus();
            
        case 0xc8:
        case 0xc9:
        case 0xca:
        case 0xcb:
            return readAgpCommand();
            
        default:
            // Fall back to standard PCI configuration reads
            return PCIHostDevice::configRead(function, reg, mem_mask);
    }
}

void SiS630HostDevice::configWrite(int function, int reg, uint32_t data, uint32_t mem_mask)
{
    // Only handle function 0 for now (host bridge)
    if (function != 0) {
        return;
    }
    
    switch (reg) {
        // SiS-specific registers
        case 0x63:
            writeDramStatus(data & 0xFF);
            break;
            
        case 0x6a:
            writeSmram(data & 0xFF);
            break;
            
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
            writeShadowRamCtrl(data);
            break;
            
        case 0x77:
            writePCIHole(data & 0xFF);
            break;
            
        case 0x9c:
            writeVgaControl(data & 0xFF);
            break;
            
        case 0xa0:
        case 0xa1:
        case 0xa2:
        case 0xa3:
            writeAgpPriorityTimer(data);
            break;
            
        case 0xa4:
        case 0xa5:
        case 0xa6:
        case 0xa7:
        case 0xa8:
        case 0xa9:
        case 0xaa:
        case 0xab:
        case 0xac:
        case 0xad:
        case 0xae:
        case 0xaf:
            writeAgpMailbox(reg - 0xa4, data & 0xFF);
            break;
            
        case 0xc8:
        case 0xc9:
        case 0xca:
        case 0xcb:
            writeAgpCommand(data);
            break;
            
        default:
            // Fall back to standard PCI configuration writes
            PCIHostDevice::configWrite(function, reg, data, mem_mask);
            break;
    }
}

void SiS630HostDevice::mapSpecialRegions(MemoryManager* memory_manager, IOManager* io_manager)
{
    // Set up RAM mapping
    updateMemoryMapping(memory_manager);
}

void SiS630HostDevice::updateMemoryMapping(MemoryManager* memory_manager)
{
    logMap("SiS Host Remapping table (shadow: %08x smram: %02x):\n", m_shadow_ram_ctrl, m_smram);
    
    // Map conventional memory (first 640KB)
    memory_manager->mapMemory(0x00000000, 0x0009FFFF, &m_ram[0x00000000/4], MemoryPermissions::ReadWrite);
    
    // Shadow RAM mapping for BIOS and other ROMs
    for (int i = 0; i < 12; i++) {
        uint32_t start_offs = 0x000c0000 + i * 0x4000;
        uint32_t end_offs = start_offs + 0x3FFF;
        
        bool read_enable = (m_shadow_ram_ctrl & (1 << i)) != 0;
        bool write_enable = (m_shadow_ram_ctrl & (1 << (i + 16))) != 0;
        
        mapShadowRAM(memory_manager, start_offs, end_offs, read_enable, write_enable);
    }
    
    // Map shadow RAM for F-segment (System BIOS)
    mapShadowRAM(
        memory_manager,
        0xF0000, 0xFFFFF,
        (m_shadow_ram_ctrl & (1 << 12)) != 0,
        (m_shadow_ram_ctrl & (1 << 28)) != 0
    );
    
    // System Management Memory Region handling
    if (m_smram & (1 << 4)) {
        uint8_t smram_config = m_smram >> 5;
        
        const uint32_t host_addresses[8] = {
            0xe0000, 0xb0000, 0xe0000, 0xb0000,
            0xe0000, 0xe0000, 0xa0000, 0xa0000
        };
        const uint32_t smram_sizes[8] = {
            0x07fff, 0xffff, 0x7fff, 0xffff,
            0x07fff, 0x7fff, 0xffff, 0x1ffff
        };
        const uint32_t system_memory_addresses[8] = {
            0xe0000, 0xb0000, 0xa0000, 0xb0000,
            0xb0000, 0xb0000, 0xa0000, 0xa0000
        };
        
        const uint32_t host_address_start = host_addresses[smram_config];
        const uint32_t host_address_end = host_address_start + smram_sizes[smram_config];
        const uint32_t system_memory_address = system_memory_addresses[smram_config];
        
        logMap("- SMRAM %02x relocation %08x-%08x to %08x\n", 
            m_smram, host_address_start, host_address_end, system_memory_address);
        
        memory_manager->mapMemory(host_address_start, host_address_end, &m_ram[system_memory_address/4], MemoryPermissions::ReadWrite);
    }
    
    // Map extended memory (above 1MB)
    memory_manager->mapMemory(0x00100000, m_ram_size - 1, &m_ram[0x00100000/4], MemoryPermissions::ReadWrite);
}

void SiS630HostDevice::mapShadowRAM(MemoryManager* memory_manager, uint32_t start_offs, uint32_t end_offs, bool read_enable, bool write_enable)
{
    logMap("- 0x%08x-0x%08x ", start_offs, end_offs);
    
    MemoryPermissions perms = MemoryPermissions::None;
    
    if (read_enable && write_enable) {
        logMap("shadow RAM r/w\n");
        perms = MemoryPermissions::ReadWrite;
    }
    else if (read_enable) {
        logMap("shadow RAM r/o\n");
        perms = MemoryPermissions::Read;
    }
    else if (write_enable) {
        logMap("shadow RAM w/o\n");
        perms = MemoryPermissions::Write;
    }
    else {
        logMap("shadow RAM off\n");
        perms = MemoryPermissions::None;
        return;
    }
    
    memory_manager->mapMemory(start_offs, end_offs, &m_ram[start_offs/4], perms);
}

void SiS630HostDevice::logShadowMemory(uint32_t data)
{
    logMap("Shadow RAM setting: 0x%08x\n", data);
    
    for (int i = 0; i < 12; i++) {
        uint32_t start_offs = 0x000C0000 + i * 0x4000;
        uint32_t end_offs = start_offs + 0x3FFF;
        
        bool read_enable = (data & (1 << i)) != 0;
        bool write_enable = (data & (1 << (i + 16))) != 0;
        
        logMap("  %08X-%08X: %s%s\n", 
            start_offs, end_offs, 
            read_enable ? "R" : "-",
            write_enable ? "W" : "-");
    }
    
    // F-segment (System BIOS)
    bool read_enable = (data & (1 << 12)) != 0;
    bool write_enable = (data & (1 << 28)) != 0;
    
    logMap("  %08X-%08X: %s%s (F-segment)\n", 
        0xF0000, 0xFFFFF, 
        read_enable ? "R" : "-",
        write_enable ? "W" : "-");
}

uint8_t SiS630HostDevice::readCapPtr()
{
    logIO("Read capptr_r [$34]\n");
    return 0xC0;  // Points to the AGP capability
}

uint8_t SiS630HostDevice::readDramStatus()
{
    logIO("Read DRAM status [$63] (%02x)\n", m_dram_status);
    return m_dram_status;
}

void SiS630HostDevice::writeDramStatus(uint8_t data)
{
    logIO("Write DRAM status [$63] %02x\n", data);
    m_dram_status = data;
    // TODO: bit 7 is shared memory control
}

uint8_t SiS630HostDevice::readSmram()
{
    logIO("Read SMRAM [$6a] (%02x)\n", m_smram);
    return m_smram;
}

void SiS630HostDevice::writeSmram(uint8_t data)
{
    logIO("Write SMRAM [$6a] %02x\n", data);
    m_smram = data;
    // Trigger remapping if needed
    // This would initiate a memory map update
}

uint32_t SiS630HostDevice::readShadowRamCtrl()
{
    logIO("Read shadow RAM setting [$70] (%08x)\n", m_shadow_ram_ctrl);
    return m_shadow_ram_ctrl;
}

void SiS630HostDevice::writeShadowRamCtrl(uint32_t data)
{
    m_shadow_ram_ctrl = data;
    logMap("Write shadow RAM setting [$70] %08x\n", data);
    logShadowMemory(data);
    // This would initiate a memory map update
}

uint8_t SiS630HostDevice::readPCIHole()
{
    logIO("Read PCI hole [$77]\n");
    return m_pci_hole;
}

void SiS630HostDevice::writePCIHole(uint8_t data)
{
    logIO("Write PCI hole [$77] %02x\n", data);
    m_pci_hole = data;
    if (data != 0) {
        Logger::GetInstance()->warn("Warning: PCI hole area enabled! %02x\n", data);
    }
}

uint8_t SiS630HostDevice::readVgaControl()
{
    logIO("Read integrated VGA control data [$9c] %02x\n", m_vga_control);
    return m_vga_control;
}

void SiS630HostDevice::writeVgaControl(uint8_t data)
{
    logIO("Write integrated VGA control data [$9c] %02x\n", data);
    m_vga_control = data;
}

// AGP interface implementation
uint32_t SiS630HostDevice::readAgpId()
{
    logAgp("Read AGP ID [$c0]\n");
    // bits 23-16 AGP v2.0
    // bits 15-8 0x00 no NEXT_PTR (NULL terminates here)
    // bits 7-0 CAP_ID (0x02 for AGP)
    return 0x00200002;
}

uint32_t SiS630HostDevice::readAgpStatus()
{
    logAgp("Read AGP status [$c4]\n");
    // bits 31-24 RQ max number of AGP command requests (0x1f + 1 = 32)
    // bit 9: SBA, side band addressing enabled
    // ---- -xxx RATE
    // ---- -1-- 4X transfer capable
    // ---- --1- 2X transfer capable
    // ---- ---1 1X transfer capable
    return 0x1f000207;
}

uint32_t SiS630HostDevice::readAgpCommand()
{
    logAgp("Read AGP command [$c8] %d %d %02x\n", m_agp.sba_enable, m_agp.enable, m_agp.data_rate);
    return (m_agp.sba_enable << 9) | (m_agp.enable << 8) | (m_agp.data_rate & 7);
}

void SiS630HostDevice::writeAgpCommand(uint32_t data)
{
    logAgp("Write AGP command [$c8] %08x\n", data);
    
    // Handle high byte (bits 8-15) - enable flags
    if ((data >> 8) & 0xFF) {
        m_agp.sba_enable = ((data >> 9) & 1) != 0;
        m_agp.enable = ((data >> 8) & 1) != 0;
        logAgp("- SBA_ENABLE = %d AGP_ENABLE = %d\n", m_agp.sba_enable, m_agp.enable);
    }
    
    // Handle low byte (bits 0-7) - data rate
    if (data & 0xFF) {
        const std::array<const char*, 8> agp_transfer_rates = {
            "(illegal 0)",
            "1X",
            "2X",
            "(illegal 3)",
            "4X",
            "(illegal 5)",
            "(illegal 6)",
            "(illegal 7)"
        };
        
        // Make sure the AGP DATA_RATE specs are honored
        const uint8_t data_rate = data & 7;
        logAgp("- DATA_RATE = %s enabled=%d\n", agp_transfer_rates[data_rate], m_agp.enable);
        m_agp.data_rate = data_rate;
    }
}

uint32_t SiS630HostDevice::readAgpPriorityTimer()
{
    logIO("Read AGP priority timer [$a0] (%08x)\n", m_agp_priority_timer);
    return m_agp_priority_timer;
}

void SiS630HostDevice::writeAgpPriorityTimer(uint32_t data)
{
    m_agp_priority_timer = data;
    logIO("Write AGP priority timer [$a0] %08x\n", data);
}

uint8_t SiS630HostDevice::readAgpMailbox(int offset)
{
    logIO("Read AGP mailbox [$%02x] (%02x)\n", offset + 0xa4, m_agp_mailbox[offset]);
    return m_agp_mailbox[offset];
}

void SiS630HostDevice::writeAgpMailbox(int offset, uint8_t data)
{
    logIO("Write AGP mailbox [$%02x] %02x\n", offset + 0xa4, data);
    m_agp_mailbox[offset] = data;
}

// Logging helpers
inline void SiS630HostDevice::logIO(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Logger::GetInstance()->logv(Logger::Level::DEBUG, format, args);
    va_end(args);
}

inline void SiS630HostDevice::logMap(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Logger::GetInstance()->logv(Logger::Level::DEBUG, format, args);
    va_end(args);
}

inline void SiS630HostDevice::logTodo(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Logger::GetInstance()->logv(Logger::Level::WARN, format, args);
    va_end(args);
}

inline void SiS630HostDevice::logAgp(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Logger::GetInstance()->logv(Logger::Level::DEBUG, format, args);
    va_end(args);
}

} // namespace x86emu