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

#ifndef X86EMULATOR_CDROM_DRIVE_H
#define X86EMULATOR_CDROM_DRIVE_H

#include "storage_device.h"
#include <cstdint>
#include <string>
#include <memory>
#include <fstream>
#include <vector>
#include <mutex>

/**
 * @brief CD-ROM drive emulation
 */
class CDROMDrive : public StorageDevice {
public:
    /**
     * @brief CD-ROM track types
     */
    enum class TrackType {
        AUDIO,
        DATA
    };
    
    /**
     * @brief CD-ROM track information
     */
    struct Track {
        TrackType type;     ///< Track type
        uint32_t startLBA;  ///< Start LBA
        uint32_t length;    ///< Length in sectors
    };
    
    /**
     * @brief Construct a new CDROMDrive
     * 
     * @param type CD-ROM drive type
     */
    explicit CDROMDrive(const std::string& type);
    
    /**
     * @brief Destroy the CDROMDrive
     */
    ~CDROMDrive() override;
    
    /**
     * @brief Initialize the CD-ROM drive
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize() override;
    
    /**
     * @brief Update the CD-ROM drive
     * 
     * @param cycles Number of CPU cycles elapsed
     */
    void update(int cycles) override;
    
    /**
     * @brief Reset the CD-ROM drive
     */
    void reset() override;
    
    /**
     * @brief Mount a CD-ROM image
     * 
     * @param path Path to the CD-ROM image
     * @return true if mounting was successful
     * @return false if mounting failed
     */
    bool mount(const std::string& path) override;
    
    /**
     * @brief Eject the CD-ROM
     */
    void eject() override;
    
    /**
     * @brief Check if a CD-ROM is mounted
     * 
     * @return true if a CD-ROM is mounted
     * @return false if no CD-ROM is mounted
     */
    bool isMounted() const override;
    
    /**
     * @brief Read data from CD-ROM
     * 
     * @param lba Logical Block Address
     * @param buffer Buffer to store data
     * @param sectorCount Number of sectors to read
     * @return true if read was successful
     * @return false if read failed
     */
    bool readData(uint32_t lba, uint8_t* buffer, uint32_t sectorCount);
    
    /**
     * @brief Read audio from CD-ROM
     * 
     * @param lba Logical Block Address
     * @param buffer Buffer to store audio data
     * @param frameCount Number of audio frames to read
     * @return true if read was successful
     * @return false if read failed
     */
    bool readAudio(uint32_t lba, uint8_t* buffer, uint32_t frameCount);
    
    /**
     * @brief Get the number of tracks
     * 
     * @return Number of tracks
     */
    uint8_t getNumTracks() const;
    
    /**
     * @brief Get track information
     * 
     * @param trackNum Track number (1-based)
     * @return Track information, or nullptr if track not found
     */
    const Track* getTrackInfo(uint8_t trackNum) const;
    
    /**
     * @brief Get the media size in sectors
     * 
     * @return Media size in sectors
     */
    uint32_t getMediaSize() const;
    
    /**
     * @brief Get the CD-ROM drive type
     * 
     * @return CD-ROM drive type
     */
    std::string getDriveType() const;

private:
    // CD-ROM drive properties
    std::string m_driveType;
    
    // CD-ROM image
    std::unique_ptr<std::fstream> m_cdromImage;
    std::vector<Track> m_tracks;
    uint32_t m_mediaSize;
    
    // CD-ROM state
    bool m_trayOpen;
    bool m_isDoorLocked;
    
    // Activity timeout
    int m_activityTimeout;
    
    // Current disc type
    enum class DiscType {
        NONE,
        ISO,
        CUE,
        BIN
    };
    DiscType m_discType;
    
    // Buffer for sector reads
    std::vector<uint8_t> m_sectorBuffer;
    
    // Helper methods
    bool openCDROMImage(const std::string& path);
    void closeCDROMImage();
    bool parseCueSheet(const std::string& path);
    bool readRawSector(uint32_t lba, uint8_t* buffer, uint32_t sectorSize);
    Track* findTrackByLBA(uint32_t lba);
    void setActivity(bool active);
};

#endif // X86EMULATOR_CDROM_DRIVE_H