#pragma once

#include "x86emulator/network_handler.h"
#include "x86emulator/network_device.h"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace x86emu {

/**
 * @brief Network manager for handling network devices and packet routing.
 */
class NetworkManager {
public:
    /**
     * Get the singleton instance.
     * @return Reference to the network manager
     */
    static NetworkManager& Instance();
    
    /**
     * Set the network handler.
     * @param handler Network handler
     */
    void SetNetworkHandler(std::shared_ptr<NetworkHandler> handler);
    
    /**
     * Get the network handler.
     * @return Network handler or nullptr if not set
     */
    std::shared_ptr<NetworkHandler> GetNetworkHandler() const;
    
    /**
     * Register a network device.
     * @param name Device name
     * @param device Network device
     */
    void RegisterDevice(const std::string& name, NetworkDevice* device);
    
    /**
     * Unregister a network device.
     * @param name Device name
     */
    void UnregisterDevice(const std::string& name);
    
    /**
     * Get a network device by name.
     * @param name Device name
     * @return Network device or nullptr if not found
     */
    NetworkDevice* GetDevice(const std::string& name) const;
    
    /**
     * Get all registered network devices.
     * @return Vector of network device pointers
     */
    std::vector<NetworkDevice*> GetAllDevices() const;
    
private:
    /**
     * Private constructor for singleton.
     */
    NetworkManager();
    
    /**
     * Private destructor for singleton.
     */
    ~NetworkManager();
    
    /**
     * Copy constructor (deleted).
     */
    NetworkManager(const NetworkManager&) = delete;
    
    /**
     * Assignment operator (deleted).
     */
    NetworkManager& operator=(const NetworkManager&) = delete;
    
    // Network handler
    std::shared_ptr<NetworkHandler> m_handler;
    
    // Registered devices (name -> device)
    std::unordered_map<std::string, NetworkDevice*> m_devices;
    
    // Access synchronization
    mutable std::mutex m_accessMutex;
    
    // Packet handler callback
    static void PacketHandlerCallback(const std::string& device, 
                                     const uint8_t* data, size_t size, 
                                     void* userData);
};

} // namespace x86emu