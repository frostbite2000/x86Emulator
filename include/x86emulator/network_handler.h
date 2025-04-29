#pragma once

#include <cstdint>
#include <string>
#include <functional>

namespace x86emu {

/**
 * @brief Packet handler callback.
 * 
 * This function is called when a packet is received by a network device.
 * @param device Device that received the packet
 * @param data Packet data
 * @param size Packet size
 * @param userData User data pointer
 */
using PacketHandlerCallback = std::function<void(const std::string& device, 
                                                const uint8_t* data, size_t size, 
                                                void* userData)>;

/**
 * @brief Network packet handler interface.
 * 
 * This interface defines methods for handling network packets.
 */
class NetworkHandler {
public:
    /**
     * Virtual destructor.
     */
    virtual ~NetworkHandler() = default;
    
    /**
     * Send a packet to the network.
     * @param device Source device name
     * @param data Packet data
     * @param size Packet size
     * @return true if the packet was sent successfully
     */
    virtual bool SendPacket(const std::string& device, const uint8_t* data, size_t size) = 0;
    
    /**
     * Register a packet handler callback.
     * @param callback Function to call when a packet is received
     * @param userData User data pointer
     */
    virtual void RegisterPacketHandler(PacketHandlerCallback callback, void* userData) = 0;
};

} // namespace x86emu