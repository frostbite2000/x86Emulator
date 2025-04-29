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

#include "floppy_drive.h"
#include "logger.h"
#include <filesystem>
#include <algorithm>

FloppyDrive::FloppyDrive(const std::string& type, int driveIndex)
    : StorageDevice("FloppyDrive_" + std::to_string(driveIndex), DeviceType::FLOPPY),
      m_driveType(driveTypeFromString(type)),
      m_driveIndex(driveIndex),
      m_tracks(0),
      m_heads(0),
      m_sectors(0),
      m_sectorSize(0),
      m_writeProtected(false),
      m_activityTimeout(0)
{
}

FloppyDrive::~FloppyDrive()
{
    // Eject any mounted disk
    eject();
}

bool FloppyDrive::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Set default geometry based on drive type
        switch (m_driveType) {
            case FloppyType::FLOPPY_360KB:
                m_tracks = 40;
                m_heads = 2;
                m_sectors = 9;
                m_sectorSize = 512;
                break;
            case FloppyType::FLOPPY_1_2MB:
                m_tracks = 80;
                m_heads = 2;
                m_sectors = 15;
                m_sectorSize = 512;
                break;
            case FloppyType::FLOPPY_720KB:
                m_tracks = 80;
                m_heads = 2;
                m_sectors = 9;
                m_sectorSize = 512;
                break;
            case FloppyType::FLOPPY_1_44MB:
                m_tracks = 80;
                m_heads = 2;
                m_sectors = 18;
                m_sectorSize = 512;
                break;
            case FloppyType::FLOPPY_2_88MB:
                m_tracks = 80;
                m_heads = 2;
                m_sectors = 36;
                m_sectorSize = 512;
                break;
        }
        
        Logger::GetInstance()->info("Floppy drive %d initialized: %s", m_driveIndex, getName().c_str());
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = ex.what();
        Logger::GetInstance()->error("Floppy drive initialization failed: %s", ex.what());
        return false;
    }
}

void FloppyDrive::update(int cycles)
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

void FloppyDrive::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Reset activity
    setActivity(false);
}

bool FloppyDrive::mount(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Close any existing disk image
    closeDiskImage();
    
    // Open the new disk image
    if (openDiskImage(path)) {
        m_mountedImagePath = path;
        Logger::GetInstance()->info("Mounted floppy disk in drive %d: %s", m_driveIndex, path.c_str());
        return true;
    }
    
    return false;
}

void FloppyDrive::eject()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Close the disk image
    closeDiskImage();
    
    // Clear mounted path
    m_mountedImagePath.clear();
    
    Logger::GetInstance()->info("Ejected floppy disk from drive %d", m_driveIndex);
}

bool FloppyDrive::isMounted() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_diskImage != nullptr && !m_mountedImagePath.empty();
}

bool FloppyDrive::readSector(int head, int track, int sector, uint8_t* buffer, size_t size)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if a disk is mounted
    if (!m_diskImage) {
        m_lastError = "No disk mounted";
        return false;
    }
    
    // Check if parameters are valid
    if (head < 0 || head >= m_heads || track < 0 || track >= m_tracks || 
        sector < 1 || sector > m_sectors || !buffer || size == 0) {
        m_lastError = "Invalid parameters";
        return false;
    }
    
    // Calculate offset
    int offset = calculateOffset(head, track, sector);
    if (offset < 0) {
        return false;
    }
    
    // Set activity
    setActivity(true);
    
    // Read data from disk image
    try {
        m_diskImage->seekg(offset, std::ios::beg);
        m_diskImage->read(reinterpret_cast<char*>(buffer), std::min(size, static_cast<size_t>(m_sectorSize)));
        
        // Set timeout for activity indicator
        m_activityTimeout = 10000; // 10,000 cycles (arbitrary value)
        
        return !m_diskImage->fail();
    }
    catch (const std::exception& ex) {
        m_lastError = ex.what();
        return false;
    }
}

bool FloppyDrive::writeSector(int head, int track, int sector, const uint8_t* buffer, size_t size)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if a disk is mounted
    if (!m_diskImage) {
        m_lastError = "No disk mounted";
        return false;
    }
    
    // Check if disk is write protected
    if (m_writeProtected) {
        m_lastError = "Disk is write protected";
        return false;
    }
    
    // Check if parameters are valid
    if (head < 0 || head >= m_heads || track < 0 || track >= m_tracks || 
        sector < 1 || sector > m_sectors || !buffer || size == 0) {
        m_lastError = "Invalid parameters";
        return false;
    }
    
    // Calculate offset
    int offset = calculateOffset(head, track, sector);
    if (offset < 0) {
        return false;
    }
    
    // Set activity
    setActivity(true);
    
    // Write data to disk image
    try {
        m_diskImage->seekp(offset, std::ios::beg);
        m_diskImage->write(reinterpret_cast<const char*>(buffer), std::min(size, static_cast<size_t>(m_sectorSize)));
        m_diskImage->flush();
        
        // Set timeout for activity indicator
        m_activityTimeout = 10000; // 10,000 cycles (arbitrary value)
        
        return !m_diskImage->fail();
    }
    catch (const std::exception& ex) {
        m_lastError = ex.what();
        return false;
    }
}

FloppyDrive::FloppyType FloppyDrive::getDriveType() const
{
    return m_driveType;
}

int FloppyDrive::getDriveIndex() const
{
    return m_driveIndex;
}

bool FloppyDrive::openDiskImage(const std::string& path)
{
    try {
        // Check if file exists
        if (!std::filesystem::exists(path)) {
            m_lastError = "File not found: " + path;
            return false;
        }
        
        // Get file size
        size_t fileSize = std::filesystem::file_size(path);
        
        // Check if the file size is valid for a floppy disk
        switch (m_driveType) {
            case FloppyType::FLOPPY_360KB:
                if (fileSize != 360 * 1024 && fileSize != 362496) {
                    m_lastError = "File size does not match 360 KB format";
                    return false;
                }
                break;
            case FloppyType::FLOPPY_1_2MB:
                if (fileSize != 1200 * 1024 && fileSize != 1228800) {
                    m_lastError = "File size does not match 1.2 MB format";
                    return false;
                }
                break;
            case FloppyType::FLOPPY_720KB:
                if (fileSize != 720 * 1024 && fileSize != 737280) {
                    m_lastError = "File size does not match 720 KB format";
                    return false;
                }
                break;
            case FloppyType::FLOPPY_1_44MB:
                if (fileSize != 1440 * 1024 && fileSize != 1474560) {
                    m_lastError = "File size does not match 1.44 MB format";
                    return false;
                }
                break;
            case FloppyType::FLOPPY_2_88MB:
                if (fileSize != 2880 * 1024 && fileSize != 2949120) {
                    m_lastError = "File size does not match 2.88 MB format";
                    return false;
                }
                break;
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
            m_writeProtected = true;
        } else {
            // File is writable
            m_writeProtected = false;
        }
        
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = ex.what();
        return false;
    }
}

void FloppyDrive::closeDiskImage()
{
    // Close the disk image
    if (m_diskImage) {
        m_diskImage->close();
        m_diskImage.reset();
    }
    
    // Clear write protection flag
    m_writeProtected = false;
}

int FloppyDrive::calculateOffset(int head, int track, int sector) const
{
    // Validate parameters
    if (head < 0 || head >= m_heads || track < 0 || track >= m_tracks || sector < 1 || sector > m_sectors) {
        m_lastError = "Invalid sector coordinates";
        return -1;
    }
    
    // Calculate offset using (cyl * heads + head) * sectors + (sector - 1)
    return ((track * m_heads + head) * m_sectors + (sector - 1)) * m_sectorSize;
}

FloppyDrive::FloppyType FloppyDrive::driveTypeFromString(const std::string& typeStr)
{
    // Convert string to floppy type
    if (typeStr == "5.25\" 360KB" || typeStr == "360KB") {
        return FloppyType::FLOPPY_360KB;
    } else if (typeStr == "5.25\" 1.2MB" || typeStr == "1.2MB") {
        return FloppyType::FLOPPY_1_2MB;
    } else if (typeStr == "3.5\" 720KB" || typeStr == "720KB") {
        return FloppyType::FLOPPY_720KB;
    } else if (typeStr == "3.5\" 2.88MB" || typeStr == "2.88MB") {
        return FloppyType::FLOPPY_2_88MB;
    } else {
        // Default to 1.44MB
        return FloppyType::FLOPPY_1_44MB;
    }
}

void FloppyDrive::setActivity(bool active)
{
    m_active = active;
    if (!active) {
        m_activityTimeout = 0;
    }
}