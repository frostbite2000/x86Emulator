#pragma once

#include "x86emulator/network_handler.h"
#include <mutex>
#include <vector>

namespace x86emu {

/**
 * @brief Loopback network handler.
 * 
 * This class implements a simple loopback network handler that routes
 * packets back to the sender for testing.
 */
class LoopbackNetworkHandler : public NetworkHandler {
public:
    /**
     * Constructor.
     */
    LoopbackNetworkHandler();
    
    /**
     * Destructor.
     */
    ~LoopbackNetworkHandler() override = default;
    
    // NetworkHandler interface implementation
    bool SendPacket(const std::string& device, const uint8_t* data, size_t size) override;
    void RegisterPacketHandler(PacketHandlerCallback callback, void* userData) override;
    
private:
    // Packet handler callback
    PacketHandlerCallback m_callback;
    void* m_callbackUserData;
    
    // Access synchronization
    std::mutex m_accessMutex;
};

} // namespace x86emu