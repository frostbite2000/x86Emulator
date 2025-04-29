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

#ifndef X86EMULATOR_HARD_DISK_H
#define X86EMULATOR_HARD_DISK_H

#include "storage_device.h"
#include <cstdint>
#include <string>
#include <memory>
#include <fstream>
#include <vector>
#include <mutex>

/**
 * @brief Hard disk emulation
 */
class HardDisk : public StorageDevice {
public:
    /**
     * @brief Hard disk geometry information
     */
    struct Geometry {
        uint16_t cylinders;    ///< Number of cylinders
        uint8_t heads;         ///< Number of heads
        uint8_t sectors;       ///< Sectors per track
        uint16_t sectorSize;   ///< Bytes per sector
    };
    
    /**
     * @brief Construct a new Hard Disk
     * 
     * @param type Disk type (e.g., "ide", "scsi")
     * @param channel Disk channel/index
     */
    HardDisk(const std::string& type, int channel);
    
    /**
     * @brief Destroy the Hard Disk
     */
    ~HardDisk() override;
    
    /**
     * @brief Initialize the hard disk
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize() override;
    
    /**
     * @brief Update the hard disk
     * 
     * @param cycles Number of CPU cycles elapsed
     */
    void update(int cycles) override;
    
    /**
     * @brief Reset the hard disk
     */
    void reset() override;
    
    /**
     * @brief Mount a hard disk image
     * 
     * @param path Path to the hard disk image
     * @return true if mounting was successful
     * @return false if mounting failed
     */
    bool mount(const std::string& path) override;
    
    /**
     * @brief Unmount the hard disk
     */
    void eject() override;
    
    /**
     * @brief Unmount the hard disk (alias for eject)
     */
    void unmount();
    
    /**
     * @brief Check if a hard disk is mounted
     * 
     * @return true if a hard disk is mounted
     * @return false if no hard disk is mounted
     */
    bool isMounted() const override;
    
    /**
     * @brief Create a new hard disk image
     * 
     * @param path Path to create the disk image
     * @param sizeInMB Size in megabytes
     * @return true if creation was successful
     * @return false if creation failed
     */
    bool create(const std::string& path, int sizeInMB);
    
    /**
     * @brief Read from hard disk
     * 
     * @param cylinder Cylinder number
     * @param head Head number
     * @param sector Sector number
     * @param buffer Buffer to store data
     * @param count Number of sectors to read
     * @return true if read was successful
     * @return false if read failed
     */
    bool read(uint16_t cylinder, uint8_t head, uint8_t sector, uint8_t* buffer, uint8_t count);
    
    /**
     * @brief Write to hard disk
     * 
     * @param cylinder Cylinder number
     * @param head Head number
     * @param sector Sector number
     * @param buffer Data to write
     * @param count Number of sectors to write
     * @return true if write was successful
     * @return false if write failed
     */
    bool write(uint16_t cylinder, uint8_t head, uint8_t sector, const uint8_t* buffer, uint8_t count);
    
    /**
     * @brief Read using LBA
     * 
     * @param lba Logical Block Address
     * @param buffer Buffer to store data
     * @param count Number of sectors to read
     * @return true if read was successful
     * @return false if read failed
     */
    bool readLBA(uint32_t lba, uint8_t* buffer, uint8_t count);
    
    /**
     * @brief Write using LBA
     * 
     * @param lba Logical Block Address
     * @param buffer Data to write
     * @param count Number of sectors to write
     * @return true if write was successful
     * @return false if write failed
     */
    bool writeLBA(uint32_t lba, const uint8_t* buffer, uint8_t count);
    
    /**
     * @brief Get disk geometry
     * 
     * @return Disk geometry information
     */
    const Geometry& getGeometry() const;
    
    /**
     * @brief Get disk size in bytes
     * 
     * @return Disk size in bytes
     */
    uint64_t getSize() const;
    
    /**
     * @brief Get disk type
     * 
     * @return Disk type
     */
    std::string getType() const;
    
    /**
     * @brief Get disk channel
     * 
     * @return Disk channel/index
     */
    int getChannel() const;

private:
    // Hard disk properties
    std::string m_diskType;
    int m_channel;
    
    // Disk geometry
    Geometry m_geometry;
    
    // Disk image
    std::unique_ptr<std::fstream> m_diskImage;
    bool m_readOnly;
    
    // Activity timeout
    int m_activityTimeout;
    
    // Helper methods
    bool openDiskImage(const std::string& path);
    void closeDiskImage();
    bool detectGeometry();
    void calculateGeometry(uint64_t size);
    uint32_t calculateOffset(uint16_t cylinder, uint8_t head, uint8_t sector) const;
    uint32_t calculateLBA(uint16_t cylinder, uint8_t head, uint8_t sector) const;
    void convertLBAtoCHS(uint32_t lba, uint16_t& cylinder, uint8_t& head, uint8_t& sector) const;
    void setActivity(bool active);
};

#endif // X86EMULATOR_HARD_DISK_H