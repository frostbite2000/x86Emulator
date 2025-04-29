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

#include "cdrom_drive.h"
#include "logger.h"
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <cctype>

// CD-ROM sector sizes
constexpr uint32_t CD_SECTOR_SIZE = 2048;         // 2048 bytes for data sectors
constexpr uint32_t CD_RAW_SECTOR_SIZE = 2352;     // 2352 bytes for raw sectors
constexpr uint32_t CD_AUDIO_FRAME_SIZE = 2352;    // 2352 bytes for audio frames

CDROMDrive::CDROMDrive(const std::string& type)
    : StorageDevice("CDROMDrive", DeviceType::CDROM),
      m_driveType(type),
      m_mediaSize(0),
      m_trayOpen(true),
      m_isDoorLocked(false),
      m_activityTimeout(0),
      m_discType(DiscType::NONE)
{
}

CDROMDrive::~CDROMDrive()
{
    // Eject any mounted disc
    eject();
}

bool CDROMDrive::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Initialize sector buffer
        m_sectorBuffer.resize(CD_RAW_SECTOR_SIZE, 0);
        
        // Close tray
        m_trayOpen = true;
        
        Logger::GetInstance()->info("CD-ROM drive initialized: %s", m_driveType.c_str());
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = ex.what();
        Logger::GetInstance()->error("CD-ROM drive initialization failed: %s", ex.what());
        return false;
    }
}

void CDROMDrive::update(int cycles)
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

void CDROMDrive::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Reset activity
    setActivity(false);
    
    // Unlock door
    m_isDoorLocked = false;
}

bool CDROMDrive::mount(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Close any existing CD-ROM image
    closeCDROMImage();
    
    // Open the new CD-ROM image
    if (openCDROMImage(path)) {
        m_mountedImagePath = path;
        m_trayOpen = false;
        Logger::GetInstance()->info("Mounted CD-ROM: %s", path.c_str());
        return true;
    }
    
    return false;
}

void CDROMDrive::eject()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if door is locked
    if (m_isDoorLocked) {
        m_lastError = "Door is locked";
        return;
    }
    
    // Close the CD-ROM image
    closeCDROMImage();
    
    // Clear mounted path
    m_mountedImagePath.clear();
    
    // Open tray
    m_trayOpen = true;
    
    Logger::GetInstance()->info("Ejected CD-ROM");
}

bool CDROMDrive::isMounted() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cdromImage != nullptr && !m_mountedImagePath.empty() && !m_trayOpen;
}

bool CDROMDrive::readData(uint32_t lba, uint8_t* buffer, uint32_t sectorCount)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if a disc is mounted
    if (!m_cdromImage || m_trayOpen) {
        m_lastError = "No disc mounted";
        return false;
    }
    
    // Check if LBA is valid
    if (lba + sectorCount > m_mediaSize) {
        m_lastError = "LBA out of range";
        return false;
    }
    
    // Set activity
    setActivity(true);
    
    // Read sectors
    for (uint32_t i = 0; i < sectorCount; i++) {
        // Find track containing this LBA
        Track* track = findTrackByLBA(lba + i);
        if (!track || track->type != TrackType::DATA) {
            m_lastError = "Invalid track type or LBA";
            return false;
        }
        
        // Read sector
        if (!readRawSector(lba + i, m_sectorBuffer.data(), CD_RAW_SECTOR_SIZE)) {
            return false;
        }
        
        // Copy data portion to output buffer
        std::memcpy(buffer + (i * CD_SECTOR_SIZE), m_sectorBuffer.data() + 16, CD_SECTOR_SIZE);
    }
    
    // Set timeout for activity indicator
    m_activityTimeout = 10000; // 10,000 cycles
    
    return true;
}

bool CDROMDrive::readAudio(uint32_t lba, uint8_t* buffer, uint32_t frameCount)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if a disc is mounted
    if (!m_cdromImage || m_trayOpen) {
        m_lastError = "No disc mounted";
        return false;
    }
    
    // Check if LBA is valid
    if (lba + frameCount > m_mediaSize) {
        m_lastError = "LBA out of range";
        return false;
    }
    
    // Set activity
    setActivity(true);
    
    // Read audio frames
    for (uint32_t i = 0; i < frameCount; i++) {
        // Find track containing this LBA
        Track* track = findTrackByLBA(lba + i);
        if (!track || track->type != TrackType::AUDIO) {
            m_lastError = "Invalid track type or LBA";
            return false;
        }
        
        // Read raw sector
        if (!readRawSector(lba + i, buffer + (i * CD_AUDIO_FRAME_SIZE), CD_AUDIO_FRAME_SIZE)) {
            return false;
        }
    }
    
    // Set timeout for activity indicator
    m_activityTimeout = 10000; // 10,000 cycles
    
    return true;
}

uint8_t CDROMDrive::getNumTracks() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<uint8_t>(m_tracks.size());
}

const CDROMDrive::Track* CDROMDrive::getTrackInfo(uint8_t trackNum) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Track numbers are 1-based
    if (trackNum == 0 || trackNum > m_tracks.size()) {
        return nullptr;
    }
    
    return &m_tracks[trackNum - 1];
}

uint32_t CDROMDrive::getMediaSize() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mediaSize;
}

std::string CDROMDrive::getDriveType() const
{
    return m_driveType;
}

bool CDROMDrive::openCDROMImage(const std::string& path)
{
    try {
        // Check if file exists
        if (!std::filesystem::exists(path)) {
            m_lastError = "File not found: " + path;
            return false;
        }
        
        // Determine file type based on extension
        std::filesystem::path filePath(path);
        std::string extension = filePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                      [](unsigned char c){ return std::tolower(c); });
        
        if (extension == ".iso") {
            // ISO image
            m_discType = DiscType::ISO;
            
            // Get file size
            size_t fileSize = std::filesystem::file_size(path);
            
            // Calculate number of sectors
            m_mediaSize = fileSize / CD_SECTOR_SIZE;
            
            // Open the file
            m_cdromImage = std::make_unique<std::fstream>();
            m_cdromImage->open(path, std::ios::in | std::ios::binary);
            
            if (!m_cdromImage->is_open()) {
                m_lastError = "Failed to open CD-ROM image";
                return false;
            }
            
            // Create a single data track
            Track track;
            track.type = TrackType::DATA;
            track.startLBA = 0;
            track.length = m_mediaSize;
            m_tracks.push_back(track);
        }
        else if (extension == ".cue") {
            // CUE sheet
            m_discType = DiscType::CUE;
            
            // Parse CUE sheet
            if (!parseCueSheet(path)) {
                return false;
            }
        }
        else if (extension == ".bin") {
            // BIN file
            m_discType = DiscType::BIN;
            
            // Get file size
            size_t fileSize = std::filesystem::file_size(path);
            
            // Assume raw sectors
            m_mediaSize = fileSize / CD_RAW_SECTOR_SIZE;
            
            // Open the file
            m_cdromImage = std::make_unique<std::fstream>();
            m_cdromImage->open(path, std::ios::in | std::ios::binary);
            
            if (!m_cdromImage->is_open()) {
                m_lastError = "Failed to open CD-ROM image";
                return false;
            }
            
            // Create a single data track
            Track track;
            track.type = TrackType::DATA;
            track.startLBA = 0;
            track.length = m_mediaSize;
            m_tracks.push_back(track);
        }
        else {
            m_lastError = "Unsupported CD-ROM image format: " + extension;
            return false;
        }
        
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = ex.what();
        return false;
    }
}

void CDROMDrive::closeCDROMImage()
{
    // Close the CD-ROM image
    if (m_cdromImage) {
        m_cdromImage->close();
        m_cdromImage.reset();
    }
    
    // Clear tracks
    m_tracks.clear();
    
    // Reset media size
    m_mediaSize = 0;
    
    // Reset disc type
    m_discType = DiscType::NONE;
}

bool CDROMDrive::parseCueSheet(const std::string& path)
{
    try {
        // Open CUE sheet
        std::ifstream cueFile(path);
        if (!cueFile) {
            m_lastError = "Failed to open CUE sheet: " + path;
            return false;
        }
        
        // Parse CUE sheet
        std::string line;
        std::string binFile;
        std::filesystem::path cuePath(path);
        std::filesystem::path cueDir = cuePath.parent_path();
        Track currentTrack;
        bool inTrack = false;
        
        while (std::getline(cueFile, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            // Skip empty lines
            if (line.empty()) {
                continue;
            }
            
            // Parse command
            std::istringstream iss(line);
            std::string command;
            iss >> command;
            
            // Convert command to uppercase
            std::transform(command.begin(), command.end(), command.begin(),
                         [](unsigned char c){ return std::toupper(c); });
            
            if (command == "FILE") {
                // Parse file name
                size_t firstQuote = line.find('"');
                size_t lastQuote = line.rfind('"');
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    binFile = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                }
                
                // Check if file type is BINARY
                std::string fileType;
                iss >> fileType;
                std::transform(fileType.begin(), fileType.end(), fileType.begin(),
                             [](unsigned char c){ return std::toupper(c); });
                
                if (fileType != "BINARY") {
                    m_lastError = "Unsupported file type in CUE sheet: " + fileType;
                    return false;
                }
                
                // Combine with directory path if not absolute
                std::filesystem::path binPath(binFile);
                if (binPath.is_relative()) {
                    binPath = cueDir / binPath;
                }
                
                // Open BIN file
                m_cdromImage = std::make_unique<std::fstream>();
                m_cdromImage->open(binPath.string(), std::ios::in | std::ios::binary);
                
                if (!m_cdromImage->is_open()) {
                    m_lastError = "Failed to open BIN file: " + binPath.string();
                    return false;
                }
                
                // Get file size
                m_cdromImage->seekg(0, std::ios::end);
                size_t fileSize = m_cdromImage->tellg();
                m_cdromImage->seekg(0, std::ios::beg);
                
                // Calculate media size
                m_mediaSize = fileSize / CD_RAW_SECTOR_SIZE;
            }
            else if (command == "TRACK") {
                // Save previous track
                if (inTrack) {
                    m_tracks.push_back(currentTrack);
                }
                
                // Parse track number
                int trackNum;
                iss >> trackNum;
                
                // Parse track type
                std::string trackType;
                iss >> trackType;
                std::transform(trackType.begin(), trackType.end(), trackType.begin(),
                             [](unsigned char c){ return std::toupper(c); });
                
                // Initialize new track
                currentTrack = Track();
                currentTrack.startLBA = 0;
                currentTrack.length = 0;
                
                if (trackType == "AUDIO") {
                    currentTrack.type = TrackType::AUDIO;
                } else {
                    currentTrack.type = TrackType::DATA;
                }
                
                inTrack = true;
            }
            else if (command == "INDEX") {
                // Parse index
                int indexNum;
                iss >> indexNum;
                
                if (indexNum == 1 && inTrack) {
                    // Parse timestamp (mm:ss:ff)
                    std::string timestamp;
                    iss >> timestamp;
                    
                    int minutes = 0, seconds = 0, frames = 0;
                    sscanf(timestamp.c_str(), "%d:%d:%d", &minutes, &seconds, &frames);
                    
                    // Convert to LBA (1 second = 75 frames)
                    currentTrack.startLBA = (minutes * 60 + seconds) * 75 + frames;
                }
            }
            else if (command == "PREGAP" || command == "POSTGAP") {
                // Ignore gap information for now
            }
        }
        
        // Save the last track
        if (inTrack) {
            m_tracks.push_back(currentTrack);
        }
        
        // Calculate track lengths
        for (size_t i = 0; i < m_tracks.size(); i++) {
            if (i < m_tracks.size() - 1) {
                // Length = next track start - this track start
                m_tracks[i].length = m_tracks[i + 1].startLBA - m_tracks[i].startLBA;
            } else {
                // Last track length = media size - track start
                m_tracks[i].length = m_mediaSize - m_tracks[i].startLBA;
            }
        }
        
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = "Error parsing CUE sheet: ";
        m_lastError += ex.what();
        return false;
    }
}

bool CDROMDrive::readRawSector(uint32_t lba, uint8_t* buffer, uint32_t sectorSize)
{
    // Check if CD-ROM is mounted
    if (!m_cdromImage || m_trayOpen) {
        m_lastError = "No disc mounted";
        return false;
    }
    
    // Check if sector size is valid
    if (sectorSize != CD_SECTOR_SIZE && sectorSize != CD_RAW_SECTOR_SIZE) {
        m_lastError = "Invalid sector size";
        return false;
    }
    
    // Check if LBA is valid
    if (lba >= m_mediaSize) {
        m_lastError = "LBA out of range";
        return false;
    }
    
    try {
        // Calculate offset based on disc type
        std::streamoff offset = 0;
        
        switch (m_discType) {
            case DiscType::ISO:
                // ISO uses 2048-byte sectors
                offset = static_cast<std::streamoff>(lba) * CD_SECTOR_SIZE;
                break;
            case DiscType::BIN:
            case DiscType::CUE:
                // BIN and CUE use 2352-byte raw sectors
                offset = static_cast<std::streamoff>(lba) * CD_RAW_SECTOR_SIZE;
                break;
            default:
                m_lastError = "Invalid disc type";
                return false;
        }
        
        // Seek to sector
        m_cdromImage->seekg(offset, std::ios::beg);
        
        // Read sector
        if (m_discType == DiscType::ISO && sectorSize == CD_RAW_SECTOR_SIZE) {
            // Reading raw sector from ISO - need to construct raw sector
            
            // Clear buffer
            std::memset(buffer, 0, CD_RAW_SECTOR_SIZE);
            
            // Read data portion
            m_cdromImage->read(reinterpret_cast<char*>(buffer + 16), CD_SECTOR_SIZE);
            
            // Add mode 1 header
            buffer[0] = 0x00;  // Sync pattern
            buffer[1] = 0xFF;
            buffer[2] = 0xFF;
            buffer[3] = 0xFF;
            buffer[4] = 0xFF;
            buffer[5] = 0xFF;
            buffer[6] = 0xFF;
            buffer[7] = 0xFF;
            buffer[8] = 0xFF;
            buffer[9] = 0xFF;
            buffer[10] = 0xFF;
            buffer[11] = 0x00;
            
            // Add address
            buffer[12] = (lba / (75 * 60)) & 0xFF;        // Minutes
            buffer[13] = ((lba / 75) % 60) & 0xFF;        // Seconds
            buffer[14] = (lba % 75) & 0xFF;               // Frames
            buffer[15] = 0x01;                            // Mode 1
        }
        else if (m_discType != DiscType::ISO && sectorSize == CD_SECTOR_SIZE) {
            // Reading data sector from BIN/CUE - need to extract data portion
            
            // Read raw sector into temporary buffer
            m_cdromImage->read(reinterpret_cast<char*>(m_sectorBuffer.data()), CD_RAW_SECTOR_SIZE);
            
            // Copy data portion
            std::memcpy(buffer, m_sectorBuffer.data() + 16, CD_SECTOR_SIZE);
        }
        else {
            // Reading matching format
            m_cdromImage->read(reinterpret_cast<char*>(buffer), sectorSize);
        }
        
        // Check for read error
        if (m_cdromImage->fail()) {
            m_lastError = "Error reading CD-ROM sector";
            return false;
        }
        
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = ex.what();
        return false;
    }
}

CDROMDrive::Track* CDROMDrive::findTrackByLBA(uint32_t lba)
{
    // Find track containing this LBA
    for (auto& track : m_tracks) {
        if (lba >= track.startLBA && lba < track.startLBA + track.length) {
            return &track;
        }
    }
    
    return nullptr;
}

void CDROMDrive::setActivity(bool active)
{
    m_active = active;
    if (!active) {
        m_activityTimeout = 0;
    }
}