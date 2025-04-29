#include "x86emulator/network/loopback_handler.h"
#include <algorithm>
#include <cassert>

namespace x86emu {

LoopbackNetworkHandler::LoopbackNetworkHandler()
    : m_callback(nullptr)
    , m_callbackUserData(nullptr)
{
}

bool LoopbackNetworkHandler::SendPacket(const std::string& device, const uint8_t* data, size_t size)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Check if we have a callback registered
    if (m_callback && data && size > 0) {
        // Make a copy of the packet data
        std::vector<uint8_t> packetCopy(data, data + size);
        
        // Call the callback with the same device name (loopback)
        m_callback(device, packetCopy.data(), packetCopy.size(), m_callbackUserData);
        
        return true;
    }
    
    return false;
}

void LoopbackNetworkHandler::RegisterPacketHandler(PacketHandlerCallback callback, void* userData)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_callback = callback;
    m_callbackUserData = userData;
}

} // namespace x86emu