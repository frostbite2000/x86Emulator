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

#include "hard_disk.h"
#include "logger.h"
#include <filesystem>
#include <algorithm>
#include <cstring>

// Constants
constexpr int DEFAULT_SECTOR_SIZE = 512;
constexpr int MAX_SECTORS_PER_TRACK = 63;
constexpr int MAX_HEADS = 16;
constexpr int MAX_CYLINDERS = 16383;  // 14-bit cylinder number (ATA limit)

HardDisk::HardDisk(const std::string& type, int channel)
    : StorageDevice("HardDisk_" + type + "_" + std::to_string(channel), DeviceType::HARDDISK),
      m_diskType(type),
      m_channel(channel),
      m_readOnly(false),
      m_activityTimeout(0)
{
    // Initialize default geometry
    m_geometry.cylinders = 0;
    m_geometry.heads = 0;
    m_geometry.sectors = 0;
    m_geometry.sectorSize = DEFAULT_SECTOR_SIZE;
}

HardDisk::~HardDisk()
{
    // Unmount any mounted disk
    unmount();
}

bool HardDisk::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Reset activity
        setActivity(false);
        
        Logger::GetInstance()->info("Hard disk initialized: %s channel %d", m_diskType.c_str(), m_channel);
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = ex.what();
        Logger::GetInstance()->error("Hard disk initialization failed: %s", ex.what());
        return false;
    }
}

void HardDisk::update(int cycles)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Update activity timeout
    if (m_active && m_activityTimeout > 0) {
        m_activityTimeout -= cycles;
        if (m_activityTimeout <= 0) {
            setActivity(false);
        }
    }
}

void HardDisk::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Reset activity
    setActivity(false);
}

bool HardDisk::mount(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Close any existing disk image
    closeDiskImage();
    
    // Open the disk image
    if (openDiskImage(path)) {
        m_mountedImagePath = path;
        
        // Detect geometry
        if (!detectGeometry()) {
            closeDiskImage();
            return false;
        }
        
        Logger::GetInstance()->info("Mounted hard disk: %s (%s channel %d)", 
                               path.c_str(), m_diskType.c_str(), m_channel);
        Logger::GetInstance()->debug("Geometry: %d cylinders, %d heads, %d sectors, %d bytes/sector",
                                m_geometry.cylinders, m_geometry.heads, m_geometry.sectors, m_geometry.sectorSize);
        
        return true;
    }
    
    return false;
}

void HardDisk::eject()
{
    unmount();
}

void HardDisk::unmount()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Close the disk image
    closeDiskImage();
    
    // Clear mounted path
    m_mountedImagePath.clear();
    
    // Reset geometry
    m_geometry.cylinders = 0;
    m_geometry.heads = 0;
    m_geometry.sectors = 0;
    
    Logger::GetInstance()->info("Unmounted hard disk from %s channel %d", m_diskType.c_str(), m_channel);
}

bool HardDisk::isMounted() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_diskImage != nullptr && !m_mountedImagePath.empty();
}

bool HardDisk::create(const std::string& path, int sizeInMB)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Check if file already exists
        if (std::filesystem::exists(path)) {
            m_lastError = "File already exists: " + path;
            return false;
        }
        
        // Check if size is valid
        if (sizeInMB <= 0 || sizeInMB > 8192) { // Limit to 8 GB
            m_lastError = "Invalid disk size";
            return false;
        }
        
        // Calculate geometry
        uint64_t size = static_cast<uint64_t>(sizeInMB) * 1024 * 1024;
        Geometry geometry;
        geometry.sectorSize = DEFAULT_SECTOR_SIZE;
        
        // Calculate CHS geometry
        calculateGeometry(size);
        
        // Create the file
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            m_lastError = "Failed to create disk image file";
            return false;
        }
        
        // Write empty data
        const int BUFFER_SIZE = 1024 * 1024; // 1 MB buffer
        std::vector<uint8_t> buffer(BUFFER_SIZE, 0);
        
        uint64_t remaining = size;
        while (remaining > 0) {
            size_t toWrite = static_cast<size_t>(std::min(static_cast<uint64_t>(BUFFER_SIZE), remaining));
            file.write(reinterpret_cast<char*>(buffer.data()), toWrite);
            
            if (file.fail()) {
                file.close();
                std::filesystem::remove(path);
                m_lastError = "Failed to write to disk image file";
                return false;
            }
            
            remaining -= toWrite;
        }
        
        file.close();
        
        Logger::GetInstance()->info("Created hard disk image: %s (%d MB)", path.c_str(), sizeInMB);
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = "Error creating hard disk image: ";
        m_lastError += ex.what();
        return false;
    }
}

bool HardDisk::read(uint16_t cylinder, uint8_t head, uint8_t sector, uint8_t* buffer, uint8_t count)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if a disk is mounted
    if (!m_diskImage) {
        m_lastError = "No disk mounted";
        return false;
    }
    
    // Check if parameters are valid
    if (cylinder >= m_geometry.cylinders || head >= m_geometry.heads || 
        sector == 0 || sector > m_geometry.sectors || !buffer || count == 0) {
        m_lastError = "Invalid parameters";
        return false;
    }
    
    // Calculate offset
    uint32_t offset = calculateOffset(cylinder, head, sector);
    if (offset == static_cast<uint32_t>(-1)) {
        return false;
    }
    
    // Set activity
    setActivity(true);
    
    // Read data from disk image
    try {
        m_diskImage->seekg(offset, std::ios::beg);
        
        for (uint8_t i = 0; i < count; i++) {
            // Read one sector
            m_diskImage->read(reinterpret_cast<char*>(buffer + i * m_geometry.sectorSize), m_geometry.sectorSize);
            
            if (m_diskImage->fail()) {
                m_lastError = "Failed to read from disk image";
                return false;
            }
            
            // Move to next sector
            if (i < count - 1) {
                sector++;
                if (sector > m_geometry.sectors) {
                    sector = 1;
                    head++;
                    if (head >= m_geometry.heads) {
                        head = 0;
                        cylinder++;
                        if (cylinder >= m_geometry.cylinders) {
                            m_lastError = "Read beyond end of disk";
                            return false;
                        }
                    }
                }
                
                // Calculate next offset
                offset = calculateOffset(cylinder, head, sector);
                if (offset == static_cast<uint32_t>(-1)) {
                    return false;
                }
                
                // Seek to next sector
                m_diskImage->seekg(offset, std::ios::beg);
            }
        }
        
        // Set timeout for activity indicator
        m_activityTimeout = 10000; // 10,000 cycles
        
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = ex.what();
        return false;
    }
}

bool HardDisk::write(uint16_t cylinder, uint8_t head, uint8_t sector, const uint8_t* buffer, uint8_t count)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if a disk is mounted
    if (!m_diskImage) {
        m_lastError = "No disk mounted";
        return false;
    }
    
    // Check if disk is read-only
    if (m_readOnly) {
        m_lastError = "Disk is read-only";
        return false;
    }
    
    // Check if parameters are valid
    if (cylinder >= m_geometry.cylinders || head >= m_geometry.heads || 
        sector == 0 || sector > m_geometry.sectors || !buffer || count == 0) {
        m_lastError = "Invalid parameters";
        return false;
    }
    
    // Calculate offset
    uint32_t offset = calculateOffset(cylinder, head, sector);
    if (offset == static_cast<uint32_t>(-1)) {
        return false;
    }
    
    // Set activity
    setActivity(true);
    
    // Write data to disk image
    try {
        m_diskImage->seekp(offset, std::ios::beg);
        
        for (uint8_t i = 0; i < count; i++) {
            // Write one sector
            m_diskImage->write(reinterpret_cast<const char*>(buffer + i * m_geometry.sectorSize), m_geometry.sectorSize);
            
            if (m_diskImage->fail()) {
                m_lastError = "Failed to write to disk image";
                return false;
            }
            
            // Move to next sector
            if (i < count - 1) {
                sector++;
                if (sector > m_geometry.sectors) {
                    sector = 1;
                    head++;
                    if (head >= m_geometry.heads) {
                        head = 0;
                        cylinder++;
                        if (cylinder >= m_geometry.cylinders) {
                            m_lastError = "Write beyond end of disk";
                            return false;
                        }
                    }
                }
                
                // Calculate next offset
                offset = calculateOffset(cylinder, head, sector);
                if (offset == static_cast<uint32_t>(-1)) {
                    return false;
                }
                
                // Seek to next sector
                m_diskImage->seekp(offset, std::ios::beg);
            }
        }
        
        // Flush data
        m_diskImage->flush();
        
        // Set timeout for activity indicator
        m_activityTimeout = 10000; // 10,000 cycles
        
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = ex.what();
        return false;
    }
}

bool HardDisk::readLBA(uint32_t lba, uint8_t* buffer, uint8_t count)
{
    // Convert LBA to CHS
    uint16_t cylinder;
    uint8_t head, sector;
    convertLBAtoCHS(lba, cylinder, head, sector);
    
    // Read using CHS
    return read(cylinder, head, sector, buffer, count);
}

bool HardDisk::writeLBA(uint32_t lba, const uint8_t* buffer, uint8_t count)
{
    // Convert LBA to CHS
    uint16_t cylinder;
    uint8_t head, sector;
    convertLBAtoCHS(lba, cylinder, head, sector);
    
    // Write using CHS
    return write(cylinder, head, sector, buffer, count);
}

const HardDisk::Geometry& HardDisk::getGeometry() const
{
    return m_geometry;
}

uint64_t HardDisk::getSize() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Calculate size from geometry
    return static_cast<uint64_t>(m_geometry.cylinders) * m_geometry.heads * m_geometry.sectors * m_geometry.sectorSize;
}

std::string HardDisk::getType() const
{
    return m_diskType;
}

int HardDisk::getChannel() const
{
    return m_channel;
}

bool HardDisk::openDiskImage(const std::string& path)
{
    try {
        // Check if file exists
        if (!std::filesystem::exists(path)) {
            m_lastError = "File not found: " + path;
            return false;
        }
        
        // Open the file
        m_diskImage = std::make_unique<std::fstream>();
        m_diskImage->open(path, std::ios::in | std::ios::out | std::ios::binary);
        
        // Check if file is writable
        if (!m_diskImage->is_open()) {
            // Try opening as read-only
            m_diskImage = std::make_unique<std::fstream>();
            m_diskImage->open(path, std::ios::in | std::ios::binary);
            
            if (!m_diskImage->is_open()) {
                m_lastError = "Failed to open disk image";
                return false;
            }
            
            // File is read-only
            m_readOnly = true;
        } else {
            // File is writable
            m_readOnly = false;
        }
        
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = "Error opening disk image: ";
        m_lastError += ex.what();
        return false;
    }
}

void HardDisk::closeDiskImage()
{
    // Close the disk image
    if (m_diskImage) {
        m_diskImage->close();
        m_diskImage.reset();
    }
    
    // Reset read-only flag
    m_readOnly = false;
}

bool HardDisk::detectGeometry()
{
    try {
        // Get file size
        m_diskImage->seekg(0, std::ios::end);
        uint64_t fileSize = m_diskImage->tellg();
        m_diskImage->seekg(0, std::ios::beg);
        
        // Calculate geometry
        calculateGeometry(fileSize);
        
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = "Error detecting disk geometry: ";
        m_lastError += ex.what();
        return false;
    }
}

void HardDisk::calculateGeometry(uint64_t size)
{
    // Calculate number of sectors
    uint32_t totalSectors = static_cast<uint32_t>(size / m_geometry.sectorSize);
    
    // Use standard geometry calculation
    
    // Start with standard values
    uint8_t heads = 16;
    uint8_t sectors = 63;
    
    // Calculate cylinders based on heads and sectors
    uint16_t cylinders = totalSectors / (heads * sectors);
    
    // Adjust if cylinders is too large
    if (cylinders > MAX_CYLINDERS) {
        if (heads < MAX_HEADS) {
            // Try with more heads first
            heads = MAX_HEADS;
            cylinders = totalSectors / (heads * sectors);
        }
        
        // If still too large, cap at maximum
        if (cylinders > MAX_CYLINDERS) {
            cylinders = MAX_CYLINDERS;
        }
    }
    
    // Set detected geometry
    m_geometry.cylinders = cylinders;
    m_geometry.heads = heads;
    m_geometry.sectors = sectors;
}

uint32_t HardDisk::calculateOffset(uint16_t cylinder, uint8_t head, uint8_t sector) const
{
    // Validate parameters
    if (cylinder >= m_geometry.cylinders || head >= m_geometry.heads || 
        sector == 0 || sector > m_geometry.sectors) {
        m_lastError = "Invalid CHS parameters";
        return static_cast<uint32_t>(-1);
    }
    
    // Calculate offset using formula: (cylinder * heads + head) * sectors_per_track + (sector - 1)
    return ((static_cast<uint32_t>(cylinder) * m_geometry.heads + head) * 
            m_geometry.sectors + (sector - 1)) * m_geometry.sectorSize;
}

uint32_t HardDisk::calculateLBA(uint16_t cylinder, uint8_t head, uint8_t sector) const
{
    // LBA = (cylinder * heads_per_cylinder + head) * sectors_per_track + (sector - 1)
    return (static_cast<uint32_t>(cylinder) * m_geometry.heads + head) * 
           m_geometry.sectors + (sector - 1);
}

void HardDisk::convertLBAtoCHS(uint32_t lba, uint16_t& cylinder, uint8_t& head, uint8_t& sector) const
{
    // C = LBA / (HPC * SPT)
    // H = (LBA / SPT) % HPC
    // S = (LBA % SPT) + 1
    
    sector = (lba % m_geometry.sectors) + 1;
    head = (lba / m_geometry.sectors) % m_geometry.heads;
    cylinder = lba / (m_geometry.heads * m_geometry.sectors);
}

void HardDisk::setActivity(bool active)
{
    m_active = active;
    if (!active) {
        m_activityTimeout = 0;
    }
}