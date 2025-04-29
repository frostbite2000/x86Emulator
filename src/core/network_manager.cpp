#include "x86emulator/network_manager.h"
#include <algorithm>
#include <cassert>

namespace x86emu {

NetworkManager& NetworkManager::Instance()
{
    static NetworkManager instance;
    return instance;
}

NetworkManager::NetworkManager()
{
}

NetworkManager::~NetworkManager()
{
    // Clear all references
    m_devices.clear();
    m_handler.reset();
}

void NetworkManager::SetNetworkHandler(std::shared_ptr<NetworkHandler> handler)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    m_handler = handler;
    
    // Register our packet handler callback
    if (m_handler) {
        m_handler->RegisterPacketHandler(PacketHandlerCallback, this);
    }
}

std::shared_ptr<NetworkHandler> NetworkManager::GetNetworkHandler() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_handler;
}

void NetworkManager::RegisterDevice(const std::string& name, NetworkDevice* device)
{
    if (!device) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Register the device
    m_devices[name] = device;
    
    // Set up a packet callback to route to our network handler
    device->RegisterPacketCallback(
        [this, name](const uint8_t* data, size_t size, void*) {
            // Get the network handler
            auto handler = this->GetNetworkHandler();
            
            // If we have a handler, send the packet
            if (handler) {
                handler->SendPacket(name, data, size);
            }
        },
        nullptr
    );
}

void NetworkManager::UnregisterDevice(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_devices.erase(name);
}

NetworkDevice* NetworkManager::GetDevice(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    auto it = m_devices.find(name);
    if (it != m_devices.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::vector<NetworkDevice*> NetworkManager::GetAllDevices() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    std::vector<NetworkDevice*> devices;
    devices.reserve(m_devices.size());
    
    for (const auto& kv : m_devices) {
        devices.push_back(kv.second);
    }
    
    return devices;
}

void NetworkManager::PacketHandlerCallback(const std::string& device, 
                                          const uint8_t* data, size_t size, 
                                          void* userData)
{
    // The userData is the NetworkManager instance
    auto* manager = static_cast<NetworkManager*>(userData);
    if (!manager) {
        return;
    }
    
    // Get all devices
    auto devices = manager->GetAllDevices();
    
    // Send the packet to all devices except the sender
    for (auto* dev : devices) {
        // Skip the sender
        if (dev->GetName() == device) {
            continue;
        }
        
        // Forward the packet to the device if it's connected
        if (dev->IsConnected()) {
            // In a real network, we would apply MAC address filtering here
            // For simplicity, we'll just forward the packet to all connected devices
            
            // Create a packet queue entry
            std::vector<uint8_t> packetCopy(data, data + size);
            
            // Queue the packet for processing on the next device update
            static_cast<RTL8139*>(dev)->m_packetQueue.push(std::move(packetCopy));
        }
    }
}

} // namespace x86emu