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

#ifndef X86EMULATOR_FLOPPY_DRIVE_H
#define X86EMULATOR_FLOPPY_DRIVE_H

#include "storage_device.h"
#include <cstdint>
#include <string>
#include <memory>
#include <fstream>
#include <vector>
#include <mutex>

/**
 * @brief Floppy drive emulation
 */
class FloppyDrive : public StorageDevice {
public:
    /**
     * @brief Floppy disk types
     */
    enum class FloppyType {
        FLOPPY_360KB,    ///< 5.25" 360 KB
        FLOPPY_1_2MB,    ///< 5.25" 1.2 MB
        FLOPPY_720KB,    ///< 3.5" 720 KB
        FLOPPY_1_44MB,   ///< 3.5" 1.44 MB
        FLOPPY_2_88MB    ///< 3.5" 2.88 MB
    };
    
    /**
     * @brief Construct a new Floppy Drive
     * 
     * @param type Floppy drive type
     * @param driveIndex Drive index (0=A:, 1=B:)
     */
    FloppyDrive(const std::string& type, int driveIndex);
    
    /**
     * @brief Destroy the Floppy Drive
     */
    ~FloppyDrive() override;
    
    /**
     * @brief Initialize the floppy drive
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize() override;
    
    /**
     * @brief Update the floppy drive
     * 
     * @param cycles Number of CPU cycles elapsed
     */
    void update(int cycles) override;
    
    /**
     * @brief Reset the floppy drive
     */
    void reset() override;
    
    /**
     * @brief Mount a floppy disk image
     * 
     * @param path Path to the floppy disk image
     * @return true if mounting was successful
     * @return false if mounting failed
     */
    bool mount(const std::string& path) override;
    
    /**
     * @brief Eject the floppy disk
     */
    void eject() override;
    
    /**
     * @brief Check if a floppy disk is mounted
     * 
     * @return true if a floppy disk is mounted
     * @return false if no floppy disk is mounted
     */
    bool isMounted() const override;
    
    /**
     * @brief Read a sector from the floppy disk
     * 
     * @param head Head number
     * @param track Track number
     * @param sector Sector number
     * @param buffer Buffer to store data
     * @param size Size to read
     * @return true if read was successful
     * @return false if read failed
     */
    bool readSector(int head, int track, int sector, uint8_t* buffer, size_t size);
    
    /**
     * @brief Write a sector to the floppy disk
     * 
     * @param head Head number
     * @param track Track number
     * @param sector Sector number
     * @param buffer Data to write
     * @param size Size to write
     * @return true if write was successful
     * @return false if write failed
     */
    bool writeSector(int head, int track, int sector, const uint8_t* buffer, size_t size);
    
    /**
     * @brief Get the floppy drive type
     * 
     * @return Floppy drive type
     */
    FloppyType getDriveType() const;
    
    /**
     * @brief Get the drive index
     * 
     * @return Drive index (0=A:, 1=B:)
     */
    int getDriveIndex() const;

private:
    // Floppy drive properties
    FloppyType m_driveType;
    int m_driveIndex;
    
    // Disk geometry
    int m_tracks;
    int m_heads;
    int m_sectors;
    int m_sectorSize;
    
    // Disk image
    std::unique_ptr<std::fstream> m_diskImage;
    std::vector<uint8_t> m_diskBuffer;
    bool m_writeProtected;
    
    // Activity timeout
    int m_activityTimeout;
    
    // Helper methods
    bool openDiskImage(const std::string& path);
    void closeDiskImage();
    int calculateOffset(int head, int track, int sector) const;
    FloppyType driveTypeFromString(const std::string& typeStr);
    void setActivity(bool active);
};

#endif // X86EMULATOR_FLOPPY_DRIVE_H