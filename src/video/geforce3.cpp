// GeForce3 (NV20) GPU emulation
// Copyright (c) 2025 x86Emulator Project

#include "geforce3.h"
#include "logger.h"
#include <cmath>
#include <algorithm>
#include <limits>

GeForce3::GeForce3()
{
    // Initialize PCI device identity
    m_vendorID = 0x10DE;      // NVIDIA
    m_deviceID = 0x0201;      // GeForce3 Ti 200 (NV20)
    m_revisionID = 0xA3;      // Implementation specific
    m_classCode = 0x030000;   // VGA Compatible Controller
    
    // Initialize PCI configuration space with known values from xboxdevwiki.net
    m_pciConfig[0x34/4] = 0x00000060;  // Capabilities pointer
    m_pciConfig[0x44/4] = 0x00200002;  // AGP status register
    m_pciConfig[0x48/4] = 0x1F000017;  // AGP command register
    m_pciConfig[0x4C/4] = 0x1F000114;  // AGP extended status
    m_pciConfig[0x60/4] = 0x00024401;  // AGP capability ID and next capability pointer

    m_fbMemBase = 0;
    m_mmioMemBase = 0;
    
    // Initialize memory and register arrays
    memset(m_pfifo, 0, sizeof(m_pfifo));
    memset(m_pcrtc, 0, sizeof(m_pcrtc));
    memset(m_pmc, 0, sizeof(m_pmc));
    memset(m_pgraph, 0, sizeof(m_pgraph));
    memset(m_ramin, 0, sizeof(m_ramin));
    memset(m_vgaRegs, 0, sizeof(m_vgaRegs));

    // Default values for key PCRTC extended registers
    m_vgaRegs[0x19] = 0x00; // REPAINT_0 - Extended Start Address and Row Offset
    m_vgaRegs[0x1A] = 0x00; // REPAINT_1
    m_vgaRegs[0x1D] = 0x00; // WRITE_BANK
    m_vgaRegs[0x1E] = 0x00; // READ_BANK
    m_vgaRegs[0x25] = 0x00; // EXTENDED_VERT
    m_vgaRegs[0x28] = 0x03; // PIXEL_FMT - default to 32bpp
    m_vgaRegs[0x2D] = 0x00; // EXTENDED_HORZ
    m_vgaRegs[0x38] = 0x00; // RMA_MODE
    m_vgaRegs[0x3E] = 0x0C; // I2C_READ - default to high
    m_vgaRegs[0x3F] = 0x30; // I2C_WRITE - default to high

    // Initialize VGA CRTC registers
    m_vgaCRTCIndex = 0x00;
    
    // Initialize DMA offsets and sizes
    for (int i = 0; i < 13; i++) {
        m_dmaOffset[i] = 0;
        m_dmaSize[i] = 0;
    }
    
    // Initialize channel state
    memset(&m_channelState, 0, sizeof(m_channelState));
    
    // Set up render state defaults
    m_renderTargetLimits.set(0, 0, 640, 480);
    m_clearRect.set(0, 0, 639, 479);
    m_renderTargetPitch = 0;
    m_depthBufferPitch = 0;
    m_renderTargetSize = 0;
    m_depthBufferSize = 0;
    
    m_log2HeightRenderTarget = 0;
    m_log2WidthRenderTarget = 0;
    m_dilateRenderTarget = 0;
    m_antialiasingRenderTarget = 0;
    
    m_renderTargetType = RenderTargetType::LINEAR;
    m_depthFormat = DepthFormat::Z24S8;
    m_colorFormat = ColorFormat::A8R8G8B8;
    m_bytesPerPixel = 4;
    
    m_antialiasControl = 0;
    m_supersampleFactorX = 1.0f;
    m_supersampleFactorY = 1.0f;
    
    m_renderTarget = nullptr;
    m_depthBuffer = nullptr;
    m_displayTarget = nullptr;
    m_oldRenderTarget = nullptr;
    
    // Initialize vertex buffers and state
    memset(&m_vertexBufferState, 0, sizeof(m_vertexBufferState));
    
    for (int n = 0; n < 4; n++) {
        m_texture[n].enabled = 0;
        m_texture[n].mode = 0;
        m_texture[n].addrModeS = 1;
        m_texture[n].addrModeT = 1;
        m_texture[n].addrModeR = 1;
        m_texture[n].buffer = nullptr;
    }
    
    m_trianglesBfCulled = 0;
    m_primitiveType = PrimitiveType::STOP;
    m_primitivesCount = 0;
    m_primitivesBatchCount = 0;
    
    m_indexesLeftCount = 0;
    m_indexesLeftFirst = 0;
    
    m_vertexCount = 0;
    m_vertexFirst = 0;
    m_vertexAccumulated = 0;
    
    // Initialize vertex attributes
    memset(&m_persistentVertexAttr, 0, sizeof(m_persistentVertexAttr));
    for (int n = 0; n < 16; n++) {
        m_persistentVertexAttr.attribute[n].fv[3] = 1.0f;
    }
    
    m_vertexInput = nullptr;
    m_vertexOutput = nullptr;
    
    // Initialize rendering state
    m_vertexPipeline = 4;  // Default to transformation matrix pipeline
    
    m_colorMask = 0xFFFFFFFF;
    m_backfaceCullingEnabled = false;
    m_backfaceCullingWinding = CullingWinding::CCW;
    m_backfaceCullingFace = CullingFace::BACK;
    
    m_alphaTestEnabled = false;
    m_depthTestEnabled = false;
    m_stencilTestEnabled = false;
    m_depthWriteEnabled = false;
    m_blendingEnabled = false;
    m_logicalOpEnabled = false;
    
    m_alphaFunc = ComparisonOp::ALWAYS;
    m_alphaReference = 0;
    m_depthFunction = ComparisonOp::LESS;
    
    m_stencilFunc = ComparisonOp::ALWAYS;
    m_stencilRef = 0;
    m_stencilMask = 0xFFFFFFFF;
    m_stencilOpFail = StencilOp::KEEP;
    m_stencilOpZFail = StencilOp::KEEP;
    m_stencilOpZPass = StencilOp::KEEP;
    
    m_blendEquation = BlendEquation::FUNC_ADD;
    m_blendFuncSource = BlendFactor::ONE;
    m_blendFuncDest = BlendFactor::ZERO;
    m_blendColor = 0x00000000;
    
    m_logicalOp = LogicalOp::COPY;
    m_fogColor = 0x00000000;
    m_bilinearFilter = false;
    
    memset(&m_bitBlit, 0, sizeof(m_bitBlit));
    
    // Initialize combiner state
    m_combinerEnabled = false;
    memset(&m_combiner, 0, sizeof(m_combiner));
    
    m_pullerWaiting = 0;
    
    m_enableWaitVblank = true;
    m_enableClippingW = false;
    
    m_rasterizer = new Rasterizer();
    
    // Initialize dilate tables
    for (int b = 0; b < 16; b++) {
        for (int a = 0; a < 2048; a++) {
            m_dilated0[b][a] = Dilate0(a, b);
            m_dilated1[b][a] = Dilate1(a, b);
        }
    }
    
    for (int b = 0; b < 16; b++) {
        for (int a = 0; a < 16; a++) {
            m_dilateChose[(b << 4) + a] = (a < b ? a : b);
        }
    }
}

GeForce3::~GeForce3()
{
    if (m_rasterizer) {
        delete m_rasterizer;
        m_rasterizer = nullptr;
    }
}

void GeForce3::InitializeAgpConfig()
{
    // Set AGP capabilities
    
    // Register 0x34: Capabilities pointer
    // Value: 0x60 - Points to the first capability structure at offset 0x60
    m_pciConfig[0x34/4] = (m_pciConfig[0x34/4] & 0xFFFFFF00) | 0x60;
    
    // Register 0x44: AGP Status
    // Value: 0x00200002
    // - Bit 1: AGP 4x supported
    // - Bit 21: AGP Fast Write supported
    m_pciConfig[0x44/4] = 0x00200002;
    
    // Register 0x48: AGP Command
    // Value: 0x1F000017
    // - Bit 0: AGP enabled
    // - Bit 1,2: AGP 4x selected
    // - Bit 4: Fast Write enabled
    // - Bits 24-28: Max AGP read queue depth
    m_pciConfig[0x48/4] = 0x1F000017;
    
    // Register 0x4C: AGP Extended Status
    // Value: 0x1F000114
    // - Bit 2: 4x supported
    // - Bit 8: Side band addressing supported
    // - Bits 24-28: Max request queue depth
    m_pciConfig[0x4C/4] = 0x1F000114;
    
    // Register 0x60: AGP Capability
    // Value: 0x00024401
    // - Byte 0: 0x01 - AGP Capability ID
    // - Byte 1: 0x44 - Pointer to next capability
    // - Bytes 2-3: 0x0002 - Version 2.0 capability
    m_pciConfig[0x60/4] = 0x00024401;
    
    LOG("AGP capabilities initialized for GeForce3 Ti 200\n");
}

bool GeForce3::Initialize()
{
    if (IsInitialized()) {
        return true;
    }
    
    // Set PCI configuration defaults
    SetVendorID(m_vendorID);
    SetDeviceID(m_deviceID);
    SetRevisionID(m_revisionID);
    SetClassCode(m_classCode);
    
    // Initialize AGP configuration
    InitializeAgpConfig();

    // Reset registers to their initial values
    Reset();
    
    // Set initialized state
    SetInitialized(true);
    
    return true;
}

void GeForce3::Reset()
{
    // Reset PCI configuration
    m_pciCommand = 0x0007;  // I/O, memory space, bus mastering enabled
    m_pciStatus = 0x02B0;   // Fast Back-to-Back, DEVSEL medium timing, 66MHz capable
    
    // Reset memory bases
    m_fbMemBase = 0;
    m_mmioMemBase = 0;
    
    // Reset all registers but preserve critical ones
    memset(m_pfifo, 0, sizeof(m_pfifo));
    memset(m_pcrtc, 0, sizeof(m_pcrtc));
    memset(m_pmc, 0, sizeof(m_pmc));
    memset(m_pgraph, 0, sizeof(m_pgraph));
    
    // Initialize RMA registers
    memset(&m_rmaRegs, 0, sizeof(m_rmaRegs));
    m_rmaRegs.configA = 0x00000000;
    m_rmaRegs.configB = 0x00000000;
    m_rmaRegs.control = 0x00000001;  // Default to enabled
    m_rmaRegs.limit = 0x01000000;    // Default memory limit
    m_rmaRegs.cursorConfig = 0x00000000;
    m_rmaRegs.cursorPos = 0x00000000;
    m_rmaRegs.fifoConfig = 0x00000001;  // FIFO enabled
    m_rmaRegs.fifoStatus = 0x00000000;

    // Set the NV_PMC_BOOT_0 register to identify as GeForce3 Ti 200
    m_pmc[0x0 / 4] = 0x020100A5;
    
    // Reset vertex buffer state
    m_vertexCount = 0;
    m_vertexFirst = 0;
    m_vertexAccumulated = 0;
    m_indexesLeftCount = 0;
    m_indexesLeftFirst = 0;
    m_primitivesCount = 0;
    m_primitivesBatchCount = 0;
    
    // Reset rendering state
    m_renderTargetType = RenderTargetType::LINEAR;
    m_depthFormat = DepthFormat::Z24S8;
    m_colorFormat = ColorFormat::A8R8G8B8;
    m_bytesPerPixel = 4;
    
    m_renderTarget = nullptr;
    m_depthBuffer = nullptr;
    
    // Reset vertex program state
    memset(&m_vertexProgram, 0, sizeof(m_vertexProgram));
    
    // Reset render states
    m_alphaTestEnabled = false;
    m_depthTestEnabled = false;
    m_stencilTestEnabled = false;
    m_depthWriteEnabled = false;
    m_blendingEnabled = false;
    m_logicalOpEnabled = false;
    
    // Reset combiner state
    memset(&m_combiner, 0, sizeof(m_combiner));
}

uint32_t GeForce3::ReadConfig(uint8_t reg, int size)
{
    uint32_t value = 0;
    
    switch (reg) {
        case 0x00:  // Vendor ID
            if (size >= 2) {
                value = m_vendorID;
            }
            break;
            
        case 0x02:  // Device ID
            if (size >= 2) {
                value = m_deviceID;
            }
            break;
            
        case 0x08:  // Revision ID and Class Code
            if (size == 1) {
                value = m_revisionID;
            } else if (size == 4) {
                value = m_revisionID | (m_classCode << 8);
            }
            break;
            
        case 0x10:  // BAR0 - Framebuffer Memory
            if (size >= 4) {
                value = m_fbMemBase;
            }
            break;
            
        case 0x14:  // BAR1 - MMIO Registers
            if (size >= 4) {
                value = m_mmioMemBase;
            }
            break;
            
        case 0x34:  // Capabilities Pointer
            if (size >= 1) {
                value = m_pciConfig[0x34/4] & 0xFF;
            }
            break;
            
        case 0x44:  // AGP Status Register
            if (size >= 4) {
                value = m_pciConfig[0x44/4];
            }
            break;
            
        case 0x48:  // AGP Command Register
            if (size >= 4) {
                value = m_pciConfig[0x48/4];
            }
            break;
            
        case 0x4C:  // AGP Extended Status
            if (size >= 4) {
                value = m_pciConfig[0x4C/4];
            }
            break;
            
        case 0x60:  // Capability ID and Next Capability Pointer
            if (size >= 4) {
                value = m_pciConfig[0x60/4];
            }
            break;
            
        // Handle other PCI configuration registers...
        default:
            // For any other registers, use parent class implementation
            value = PCIDevice::ReadConfig(reg, size);
            break;
    }
    
    return value;
}

void GeForce3::WriteConfig(uint8_t reg, uint32_t value, int size)
{
    switch (reg) {
        case 0x04:  // Command register
            if (size >= 2) {
                m_pciCommand = value & 0xFFFF;
            }
            break;
            
        case 0x10:  // BAR0 - Framebuffer Memory
            if (size >= 4) {
                // Only the bits that are not hardwired to 0 can be written
                // Typically memory BARs are aligned to their size
                // For a 64MB frame buffer, the lower 26 bits would be read-only 0
                m_fbMemBase = (m_fbMemBase & 0x03FFFFFF) | (value & 0xFC000000);
            }
            break;
            
        case 0x14:  // BAR1 - MMIO Registers
            if (size >= 4) {
                // For a 16MB register space, the lower 24 bits would be read-only 0
                m_mmioMemBase = (m_mmioMemBase & 0x00FFFFFF) | (value & 0xFF000000);
            }
            break;
            
        case 0x48:  // AGP Command Register
            if (size >= 4) {
                // Only certain bits are writable in the AGP command register
                // Typically bits controlling AGP transfer rate, sideband addressing, etc.
                uint32_t writableMask = 0x1F000017; // Based on observed value
                m_pciConfig[0x48/4] = (m_pciConfig[0x48/4] & ~writableMask) | (value & writableMask);
                
                LOG("AGP Command Register set to %08X\n", m_pciConfig[0x48/4]);
            }
            break;
            
        // Handle other PCI configuration registers...
        default:
            // For any other registers, use parent class implementation
            PCIDevice::WriteConfig(reg, value, size);
            break;
    }
}

void GeForce3::UpdateRmaFramebufferConfig()
{
    // Process RMA Config A register to update framebuffer settings
    uint32_t configA = m_rmaRegs.configA;
    
    // Extract relevant bits from configA
    bool enableRma = (configA & 0x01) != 0;
    uint32_t pixelFormat = (configA >> 8) & 0x7;
    uint32_t pitch = ((configA >> 16) & 0xFF) * 256; // Pitch is in 256-byte units
    
    if (enableRma) {
        // Update framebuffer configuration based on RMA settings
        switch (pixelFormat) {
            case 0: // 8bpp indexed
                m_colorFormat = ColorFormat::B8;
                m_bytesPerPixel = 1;
                break;
                
            case 1: // 15bpp RGB (5-5-5)
                m_colorFormat = ColorFormat::X1R5G5B5_X1R5G5B5;
                m_bytesPerPixel = 2;
                break;
                
            case 2: // 16bpp RGB (5-6-5)
                m_colorFormat = ColorFormat::R5G6B5;
                m_bytesPerPixel = 2;
                break;
                
            case 3: // 24bpp RGB (8-8-8)
                // GeForce3 doesn't have native 24bpp support, treat as 32bpp
                m_colorFormat = ColorFormat::X8R8G8B8_X8R8G8B8;
                m_bytesPerPixel = 4;
                break;
                
            case 4: // 32bpp ARGB (8-8-8-8)
                m_colorFormat = ColorFormat::A8R8G8B8;
                m_bytesPerPixel = 4;
                break;
                
            default:
                LOG("Unsupported RMA pixel format: %d\n", pixelFormat);
                break;
        }
        
        // Update render target pitch if specified in RMA
        if (pitch > 0) {
            m_renderTargetPitch = pitch;
            UpdateRenderTargetSize();
        }
    }
}

uint32_t GeForce3::ReadMemory(uint32_t address, int size)
{
    // Check if memory space is enabled
    if (!(m_pciCommand & 0x0002)) {
        return 0xFFFFFFFF;
    }
    
    // Handle framebuffer access
    if (address >= m_fbMemBase && address < m_fbMemBase + (256 * 1024 * 1024)) {
        uint32_t offset = address - m_fbMemBase;
        // Read from framebuffer memory
        if (m_ramBase) {
            if (offset + size <= m_ramSize) {
                if (size == 4)
                    return *reinterpret_cast<uint32_t*>(m_ramBase + offset);
                else if (size == 2)
                    return *reinterpret_cast<uint16_t*>(m_ramBase + offset);
                else 
                    return *(m_ramBase + offset);
            }
        }
        return 0xFFFFFFFF;
    }
    // Handle MMIO register access
    else if (address >= m_mmioMemBase && address < m_mmioMemBase + (16 * 1024 * 1024)) {
        uint32_t regAddress = address - m_mmioMemBase;
        return ReadRegister(regAddress);
    }
    else if ((address >= m_mmioMemBase + 0x601000) && (address < m_mmioMemBase + 0x602000)) {
        // PRMCIO - VGA CRTC and attribute controller
        switch ((address - m_mmioMemBase - 0x601000) & 0xFFF) {
            case 0x3D4: // CRTC_INDEX
                return m_vgaCRTCIndex;
                
            case 0x3D5: // CRTC_DATA
                if (m_vgaCRTCIndex < 0x100)
                    return m_vgaRegs[m_vgaCRTCIndex];
                return 0xFF;
                
            case 0x3D0: // RMA_ACCESS
                if (m_vgaRegs[0x38] & 0x01) {
                    uint32_t rmaReg = (m_vgaRegs[0x38] >> 1) & 0x07;
                    
                    // Return the currently addressed RMA register
                    // For now just return 0, you'll need to implement proper RMA register handling
                    return 0;
                }
                return 0xFF;
                
            default:
                LOG("Unhandled VGA CRTC read from offset %03X\n", 
                    (address - m_mmioMemBase - 0x601000) & 0xFFF);
                return 0xFF;
        }
    }
    
    return 0xFFFFFFFF;
}

void GeForce3::WriteMemory(uint32_t address, uint32_t value, int size)
{
    // Check if memory space is enabled
    if (!(m_pciCommand & 0x0002)) {
        return;
    }
    
    // Handle framebuffer access
    if (address >= m_fbMemBase && address < m_fbMemBase + (256 * 1024 * 1024)) {
        uint32_t offset = address - m_fbMemBase;
        // Write to framebuffer memory
        if (m_ramBase) {
            if (offset + size <= m_ramSize) {
                if (size == 4)
                    *reinterpret_cast<uint32_t*>(m_ramBase + offset) = value;
                else if (size == 2)
                    *reinterpret_cast<uint16_t*>(m_ramBase + offset) = (uint16_t)value;
                else
                    *(m_ramBase + offset) = (uint8_t)value;
            }
        }
    }
    // Handle MMIO register access
    else if (address >= m_mmioMemBase && address < m_mmioMemBase + (16 * 1024 * 1024)) {
        uint32_t regAddress = address - m_mmioMemBase;
        WriteRegister(regAddress, value);
    }
    else if ((address >= m_mmioMemBase + 0x601000) && (address < m_mmioMemBase + 0x602000)) {
        // PRMCIO - VGA CRTC and attribute controller
        switch ((address - m_mmioMemBase - 0x601000) & 0xFFF) {
            case 0x3D4: // CRTC_INDEX
                m_vgaCRTCIndex = value & 0xFF;
                break;
                
            case 0x3D5: // CRTC_DATA
                if (m_vgaCRTCIndex < 0x100) {
                    uint8_t oldValue = m_vgaRegs[m_vgaCRTCIndex];
                    m_vgaRegs[m_vgaCRTCIndex] = value & 0xFF;
                    
                    // Handle special register updates
                    switch (m_vgaCRTCIndex) {
                        case 0x28: // PIXEL_FMT
                            {
                                // Update the framebuffer format based on the pixel format register
                                // Only do this if RMA is not controlling the framebuffer
                                if (!(m_rmaRegs.configA & 0x01)) {
                                    uint8_t pixelFormat = value & 0x03;
                                    switch (pixelFormat) {
                                        case 0: // VGA
                                            // Handle VGA mode
                                            break;
                                        case 1: // 8bpp
                                            m_colorFormat = ColorFormat::B8;
                                            m_bytesPerPixel = 1;
                                            break;
                                        case 2: // 16bpp
                                            m_colorFormat = ColorFormat::R5G6B5;
                                            m_bytesPerPixel = 2;
                                            break;
                                        case 3: // 32bpp
                                            m_colorFormat = ColorFormat::A8R8G8B8;
                                            m_bytesPerPixel = 4;
                                            break;
                                    }
                                }
                            }
                            break;
                            
                        case 0x38: // RMA_MODE
                            {
                                // The RMA mode register controls access to RMA registers
                                bool oldRmaEnabled = (oldValue & 0x01) != 0;
                                bool newRmaEnabled = (value & 0x01) != 0;
                                
                                if (!oldRmaEnabled && newRmaEnabled) {
                                    LOG("RMA access port enabled\n");
                                } else if (oldRmaEnabled && !newRmaEnabled) {
                                    LOG("RMA access port disabled\n");
                                }
                                
                                // RMA register index changed
                                uint8_t oldRmaReg = (oldValue >> 1) & 0x07;
                                uint8_t newRmaReg = (value >> 1) & 0x07;
                                
                                if (newRmaEnabled && oldRmaReg != newRmaReg) {
                                    LOG("RMA register selection changed from %d to %d\n", 
                                        oldRmaReg, newRmaReg);
                                }
                            }
                            break;
                    }
                    
                    LOG("VGA CRTC[%02X] = %02X\n", m_vgaCRTCIndex, value & 0xFF);
                }
                break;
                
            case 0x3D0: // RMA_ACCESS
                if (m_vgaRegs[0x38] & 0x01) {
                    uint32_t rmaReg = (m_vgaRegs[0x38] >> 1) & 0x07;
                    uint32_t rmaValue = value;
                    
                    // Implement RMA register write
                    // This requires knowing which RMA registers exist and how they work
                    LOG("RMA register %d write: %08X\n", rmaReg, rmaValue);
                }
                break;
                
            default:
                LOG("Unhandled VGA CRTC write to offset %03X: %08X\n", 
                    (address - m_mmioMemBase - 0x601000) & 0xFFF, value);
                break;
        }
    }
}

uint32_t GeForce3::ReadRegister(uint32_t address)
{
    uint32_t ret = 0;
    
    if ((address >= 0x00100000) && (address < 0x00101000)) {
        // PFB registers
        ret = 0;
        if (address == 0x100200)
            return 3;
    }
    else if ((address >= 0x00101000) && (address < 0x00102000)) {
        // STRAPS registers
    }
    else if ((address >= 0x00002000) && (address < 0x00004000)) {
        uint32_t offset = (address - 0x00002000) / 4;
        ret = m_pfifo[offset];
        
        // PFIFO.CACHE1.STATUS or PFIFO.RUNOUT_STATUS
        if ((address == 0x3214) || (address == 0x2400))
            ret = 0x10;
    }
    else if ((address >= 0x00700000) && (address < 0x00800000)) {
        uint32_t offset = (address - 0x00700000) / 4;
        ret = m_ramin[offset];
    }
    else if ((address >= 0x00400000) && (address < 0x00402000)) {
        uint32_t offset = (address - 0x00400000) / 4;
        ret = m_pgraph[offset];
    }
    else if ((address >= 0x00600000) && (address < 0x00601000)) {
        uint32_t offset = (address - 0x00600000) / 4;
        ret = m_pcrtc[offset];
    }
    else if ((address >= 0x00000000) && (address < 0x00001000)) {
        uint32_t offset = (address - 0x00000000) / 4;
        ret = m_pmc[offset];
    }
    else if ((address >= 0x00800000) && (address < 0x00900000)) {
        // 32 channels size 0x10000 each, 8 subchannels per channel size 0x2000 each
        uint32_t suboffset = (address - 0x00800000) / 4;
        uint32_t channel = (suboffset >> (16 - 2)) & 31;
        uint32_t subchannel = (suboffset >> (13 - 2)) & 7;
        suboffset = suboffset & 0x7ff;
        
        if (suboffset < 0x80 / 4)
            ret = m_channelState.channel[channel][subchannel].regs[suboffset];
        
        return ret;
    }
    
    return ret;
}

void GeForce3::WriteRegister(uint32_t address, uint32_t value)
{
    bool updateInt = false;
    
    if ((address >= 0x00101000) && (address < 0x00102000)) {
        // STRAPS registers
    }
    else if ((address >= 0x00002000) && (address < 0x00004000)) {
        uint32_t offset = (address - 0x00002000) / 4;
        if (offset < (sizeof(m_pfifo) / sizeof(uint32_t))) {
            m_pfifo[offset] = value;
        }
    }
    else if ((address >= 0x00700000) && (address < 0x00800000)) {
        uint32_t offset = (address - 0x00700000) / 4;
        if (offset < (sizeof(m_ramin) / sizeof(uint32_t))) {
            m_ramin[offset] = value;
        }
    }
    else if ((address >= 0x00400000) && (address < 0x00402000)) {
        uint32_t offset = (address - 0x00400000) / 4;
        if (offset < (sizeof(m_pgraph) / sizeof(uint32_t))) {
            uint32_t old = m_pgraph[offset];
            m_pgraph[offset] = value;
            
            if (offset == 0x100 / 4) {
                m_pgraph[offset] = old & ~value;
                if (value & 1)
                    m_pgraph[0x108 / 4] = 0;
                updateInt = true;
            }
            
            if (offset == 0x140 / 4)
                updateInt = true;
                
            if (offset == 0x720 / 4) {
                if ((value & 1) && (m_pullerWaiting == 2)) {
                    m_pullerWaiting = 0;
                    ProcessGPUCommands();
                }
            }
            
            // Clear read-only registers
            if ((offset >= 0x900 / 4) && (offset < 0xa00 / 4))
                m_pgraph[offset] = 0;
        }
    }
    else if ((address >= 0x00600000) && (address < 0x00601000)) {
        uint32_t offset = (address - 0x00600000) / 4;
        if (offset < (sizeof(m_pcrtc) / sizeof(uint32_t))) {
            uint32_t old = m_pcrtc[offset];
            m_pcrtc[offset] = value;
            
            if (offset == 0x100 / 4) {
                m_pcrtc[offset] = old & ~value;
                updateInt = true;
            }
            
            if (offset == 0x140 / 4)
                updateInt = true;
                
            if (offset == 0x800 / 4) {
                m_displayTarget = (uint32_t*)DirectAccessPtr(value);
                LOG("CRTC buffer set to %08X\n", value);
            }
        }
    }
    else if ((address >= 0x00000000) && (address < 0x00001000)) {
        uint32_t offset = (address - 0x00000000) / 4;
        if (offset < (sizeof(m_pmc) / sizeof(uint32_t))) {
            m_pmc[offset] = value;
            
            if (offset == 0x200 / 4) { // PMC.ENABLE register
                if (value & 0x1100) { // either PFIFO or PGRAPH enabled
                    for (int ch = 0; ch < 32; ch++) {
                        // zero dma_get in all the channels
                        m_channelState.channel[ch][0].regs[0x44 / 4] = 0;
                    }
                }
            }
        }
    }
    else if ((address >= 0x00800000) && (address < 0x00900000)) {
        // 32 channels size 0x10000 each, 8 subchannels per channel size 0x2000 each
        uint32_t suboffset = (address - 0x00800000) / 4;
        uint32_t channel = (suboffset >> (16 - 2)) & 31;
        uint32_t subchannel = (suboffset >> (13 - 2)) & 7;
        suboffset = suboffset & 0x7ff;
        
        m_channelState.channel[channel][subchannel].regs[suboffset] = value;
        
        if (suboffset >= 0x80 / 4)
            return;
            
        if ((suboffset == 0x40 / 4) || (suboffset == 0x44 / 4)) {
            uint32_t* dmaput = &m_channelState.channel[channel][0].regs[0x40 / 4];
            uint32_t* dmaget = &m_channelState.channel[channel][0].regs[0x44 / 4];
            
            if (*dmaget != *dmaput) {
                if (m_pullerWaiting == 0) {
                    m_pullerWaiting = 0;
                    ProcessGPUCommands();
                }
            }
        }
    }
    
    if (updateInt) {
        if (UpdateInterrupts()) {
            if (m_irqCallback) {
                m_irqCallback(1);  // Signal IRQ active
            }
        } else {
            if (m_irqCallback) {
                m_irqCallback(0);  // Signal IRQ inactive
            }
        }
    }
}

void GeForce3::ExecuteMethod(uint32_t channelID, uint32_t subchannelID, uint32_t method, uint32_t parameter)
{
    // Store the method data in the channel's object
    if (method / 4 < sizeof(m_channelState.channel[channelID].object[subchannelID].method) / sizeof(uint32_t)) {
        m_channelState.channel[channelID].object[subchannelID].method[method / 4] = parameter;
    }
    
    // Execute the method based on the object class
    uint32_t objClass = m_channelState.channel[channelID].object[subchannelID].objclass;
    
    switch (objClass) {
        case 0x97:  // 3D graphics context
            ExecuteMethodGraphics(channelID, subchannelID, method, parameter);
            break;
            
        case 0x39:  // Memory to Memory Format (M2MF)
            ExecuteMethodM2MF(channelID, subchannelID, method, parameter);
            break;
            
        case 0x62:  // 2D Surface
            ExecuteMethodSurf2D(channelID, subchannelID, method, parameter);
            break;
            
        case 0x9F:  // Blit
            ExecuteMethodBlit(channelID, subchannelID, method, parameter);
            break;
            
        // Other object classes
        default:
            LOG("Unhandled object class: 0x%02X, method: 0x%04X, param: 0x%08X\n", 
                objClass, method, parameter);
            break;
    }
}

void GeForce3::ExecuteMethodGraphics(uint32_t channelID, uint32_t subchannelID, uint32_t method, uint32_t parameter)
{
    switch (method) {
        // Vertex program methods
        case 0x1E94: // VP_UPLOAD_FROM_ID
            m_vertexProgram.uploadInstructionIndex = parameter;
            m_vertexProgram.uploadInstructionComponent = 0;
            break;
            
        case 0x1EA0: // VP_START_FROM_ID
            m_vertexProgram.instructions = m_vertexProgram.uploadInstructionIndex;
            m_vertexProgram.startInstruction = parameter;
            break;
            
        case 0x1EA4: // VP_UPLOAD_CONST_ID
            m_vertexProgram.uploadParameterIndex = parameter;
            m_vertexProgram.uploadParameterComponent = 0;
            break;
            
        // Vertex shader instructions upload (0x0B00-0x0B7C)
        case 0x0B00 ... 0x0B7C: {
            int index = (method - 0x0B00) / 4;
            if (m_vertexProgram.uploadInstructionIndex < 256) {
                m_vertexProgram.instructions[m_vertexProgram.uploadInstructionIndex].data[m_vertexProgram.uploadInstructionComponent] = parameter;
                m_vertexProgram.instructions[m_vertexProgram.uploadInstructionIndex].modified |= (1 << m_vertexProgram.uploadInstructionComponent);
            }
            
            if (m_vertexProgram.instructions[m_vertexProgram.uploadInstructionIndex].modified == 15) {
                m_vertexProgram.instructions[m_vertexProgram.uploadInstructionIndex].modified = 0;
                DecodeVertexInstruction(m_vertexProgram.uploadInstructionIndex);
            }
            
            m_vertexProgram.uploadInstructionComponent++;
            if (m_vertexProgram.uploadInstructionComponent >= 4) {
                m_vertexProgram.uploadInstructionComponent = 0;
                m_vertexProgram.uploadInstructionIndex++;
            }
            break;
        }
            
        // Vertex shader constants upload (0x0B80-0x0BFC)
        case 0x0B80 ... 0x0BFC: {
            int index = (method - 0x0B80) / 4;
            if (m_vertexProgram.uploadParameterIndex < 192) {
                m_vertexProgram.constants[m_vertexProgram.uploadParameterIndex].SetComponent(m_vertexProgram.uploadParameterComponent, parameter);
            }
            
            m_vertexProgram.uploadParameterComponent++;
            if (m_vertexProgram.uploadParameterComponent >= 4) {
                m_vertexProgram.uploadParameterComponent = 0;
                m_vertexProgram.uploadParameterIndex++;
            }
            break;
        }
        
        // Drawing methods
        case 0x17FC: // BEGIN_END
            HandleDrawCommand(parameter);
            break;
            
        case 0x1800:  // Draw vertices (word indices)
        case 0x1808:  // Draw vertices (dword indices)
            HandleDrawIndices(method == 0x1800 ? 2 : 1, parameter);
            break;
            
        case 0x1810:  // Draw vertices with offset
            HandleDrawVerticesWithOffset(parameter);
            break;
            
        case 0x1818:  // Draw raw vertices
            HandleDrawRawVertices(parameter);
            break;
            
        // Texture methods
        case 0x1B00 ... 0x1BFC:
            HandleTextureMethod(method, parameter);
            break;
            
        // Matrix methods
        case 0x0440 ... 0x047C: // Projection matrix
            {
                int matrixIndex = (method - 0x0440) / 4;
                int row = matrixIndex / 4;
                int col = matrixIndex % 4;
                m_matrices.projection[row][col] = *(float*)&parameter;
            }
            break;
            
        case 0x0480 ... 0x04BC: // Modelview matrix
            {
                int matrixIndex = (method - 0x0480) / 4;
                int row = matrixIndex / 4;
                int col = matrixIndex % 4;
                m_matrices.modelview[row][col] = *(float*)&parameter;
            }
            break;
            
        case 0x0580 ... 0x05BC: // Inverse modelview matrix
            {
                int matrixIndex = (method - 0x0580) / 4;
                int row = matrixIndex / 4;
                int col = matrixIndex % 4;
                m_matrices.modelviewInverse[row][col] = *(float*)&parameter;
            }
            break;
            
        case 0x0680 ... 0x06BC: // Composite matrix
            {
                int matrixIndex = (method - 0x0680) / 4;
                int row = matrixIndex / 4;
                int col = matrixIndex % 4;
                m_matrices.composite[row][col] = *(float*)&parameter;
            }
            break;
            
        // Viewport/Transform methods
        case 0x0A20 ... 0x0A2C: // Viewport translate
            {
                int index = (method - 0x0A20) / 4;
                m_matrices.translate[index] = *(float*)&parameter;
                
                // Set corresponding vertex shader constant too
                m_vertexProgram.constants[59].SetComponent(index, parameter); // constant -37
            }
            break;
            
        case 0x0AF0 ... 0x0AFC: // Viewport scale
            {
                int index = (method - 0x0AF0) / 4;
                m_matrices.scale[index] = *(float*)&parameter;
                
                // Set corresponding vertex shader constant too
                m_vertexProgram.constants[58].SetComponent(index, parameter); // constant -38
            }
            break;
            
        // Render state methods
        case 0x0200: // Set render target dimensions
            HandleRenderTargetDimensions(parameter);
            break;
            
        case 0x0204: // Set render target height
            HandleRenderTargetHeight(parameter);
            break;
            
        case 0x0208: // Set render target format
            HandleRenderTargetFormat(parameter);
            break;
            
        case 0x020C: // Set render target pitch
            m_renderTargetPitch = parameter & 0xFFFF;
            m_depthBufferPitch = (parameter >> 16) & 0xFFFF;
            UpdateRenderTargetSize();
            LOG("Pitch color %04X zbuffer %04X\n", m_renderTargetPitch, m_depthBufferPitch);
            break;
            
        case 0x0210: // Set framebuffer offset
            m_oldRenderTarget = m_renderTarget;
            m_renderTarget = (uint32_t*)DirectAccessPtr(parameter);
            LOG("Render target at %08X\n", parameter);
            break;
            
        case 0x0214: // Set depth buffer offset
            m_depthBuffer = (uint32_t*)DirectAccessPtr(parameter);
            LOG("Depth buffer at %08X\n", parameter);
            if ((parameter == 0) || (parameter > 0x7FFFFFFC))
                m_depthWriteEnabled = false;
            else if (m_channelState.channel[channelID].object[subchannelID].method[0x035C / 4] != 0)
                m_depthWriteEnabled = true;
            else
                m_depthWriteEnabled = false;
            break;
            
        // Color clearing
        case 0x1D8C: // Depth clear value
            // Used for clearing depth buffer and for interrupt routines
            m_pgraph[0x1A88 / 4] = parameter;
            break;
            
        case 0x1D90: // Color clear value
            // Used for clearing color buffer and for interrupt routines
            m_pgraph[0x186C / 4] = parameter;
            break;
            
        case 0x1D94: // Clear framebuffer
            // Clear specified buffers: color, depth, stencil
            clear_render_target((parameter >> 4) & 15, 
                               m_channelState.channel[channelID].object[subchannelID].method[0x1D90 / 4]);
            clear_depth_buffer(parameter & 3, 
                              m_channelState.channel[channelID].object[subchannelID].method[0x1D8C / 4]);
            break;
            
        case 0x1D98: // Set clear rect X
            {
                int x = parameter & 0xFFFF;
                int w = (parameter >> 16) & 0xFFFF;
                m_clearRect.setx(x, w);
            }
            break;
            
        case 0x1D9C: // Set clear rect Y
            {
                int y = parameter & 0xFFFF;
                int h = (parameter >> 16) & 0xFFFF;
                m_clearRect.sety(y, h);
            }
            break;
            
        // Clipping windows
        case 0x02C0 ... 0x02DC: // Clipping window X dimensions
            {
                int i = (method - 0x2C0) / 4;
                int x = parameter & 0xFFFF;
                int w = (parameter >> 16) & 0xFFFF;
                m_clippingWindows[i].setx(x, x + w - 1);
            }
            break;
            
        case 0x02E0 ... 0x02FC: // Clipping window Y dimensions
            {
                int i = (method - 0x2E0) / 4;
                int y = parameter & 0xFFFF;
                int h = (parameter >> 16) & 0xFFFF;
                m_clippingWindows[i].sety(y, y + h - 1);
            }
            break;
            
        // Rasterization state
        case 0x0308: // Backface culling enable
            m_backfaceCullingEnabled = parameter != 0;
            break;
            
        case 0x03A0: // Backface culling winding
            m_backfaceCullingWinding = static_cast<CullingWinding>(parameter);
            break;
            
        case 0x039C: // Backface culling face
            m_backfaceCullingFace = static_cast<CullingFace>(parameter);
            break;
            
        // Alpha test
        case 0x0300: // Alpha test enable
            m_alphaTestEnabled = parameter != 0;
            break;
            
        case 0x033C: // Alpha test function
            m_alphaFunc = static_cast<ComparisonOp>(parameter);
            break;
            
        case 0x0340: // Alpha test reference
            m_alphaReference = parameter;
            break;
            
        // Depth test
        case 0x030C: // Depth test enable
            m_depthTestEnabled = parameter != 0;
            break;
            
        case 0x0354: // Depth function
            m_depthFunction = static_cast<ComparisonOp>(parameter);
            break;
            
        case 0x035C: // Depth write enable
            {
                uint32_t depthAddr = m_channelState.channel[channelID].object[subchannelID].method[0x0214 / 4];
                m_depthWriteEnabled = parameter != 0;
                if ((depthAddr == 0) || (depthAddr > 0x7FFFFFFC))
                    m_depthWriteEnabled = false;
            }
            break;
            
        // Write masks
        case 0x0358: // Color mask
            {
                uint32_t tempMask = 0;
                if (parameter & 0x000000FF)
                    tempMask |= 0x000000FF;
                if (parameter & 0x0000FF00)
                    tempMask |= 0x0000FF00;
                if (parameter & 0x00FF0000)
                    tempMask |= 0x00FF0000;
                if (parameter & 0xFF000000)
                    tempMask |= 0xFF000000;
                m_colorMask = tempMask;
            }
            break;
            
        // Stencil test
        case 0x032C: // Stencil test enable
            m_stencilTestEnabled = parameter != 0;
            break;
            
        case 0x0364: // Stencil function
            m_stencilFunc = static_cast<ComparisonOp>(parameter);
            break;
            
        case 0x0368: // Stencil reference value
            m_stencilRef = parameter > 255 ? 255 : parameter;
            break;
            
        case 0x036C: // Stencil mask
            m_stencilMask = parameter;
            break;
            
        case 0x0370: // Stencil op fail
            m_stencilOpFail = static_cast<StencilOp>(parameter);
            break;
            
        case 0x0374: // Stencil op zfail
            m_stencilOpZFail = static_cast<StencilOp>(parameter);
            break;
            
        case 0x0378: // Stencil op zpass
            m_stencilOpZPass = static_cast<StencilOp>(parameter);
            break;
            
        // Blending
        case 0x0304: // Blend enable
            if (m_logicalOpEnabled)
                m_blendingEnabled = false;
            else
                m_blendingEnabled = parameter != 0;
            break;
            
        case 0x0344: // Blend function source
            m_blendFuncSource = static_cast<BlendFactor>(parameter);
            break;
            
        case 0x0348: // Blend function destination
            m_blendFuncDest = static_cast<BlendFactor>(parameter);
            break;
            
        case 0x034C: // Blend color
            m_blendColor = parameter;
            break;
            
        case 0x0350: // Blend equation
            m_blendEquation = static_cast<BlendEquation>(parameter);
            break;
            
        // Logical operations
        case 0x0D40: // Logical operation enable
            if (parameter != 0)
                m_blendingEnabled = false;
            else
                m_blendingEnabled = m_channelState.channel[channelID].object[subchannelID].method[0x0304 / 4] != 0;
            m_logicalOpEnabled = parameter != 0;
            break;
            
        case 0x0D44: // Logical operation
            m_logicalOp = static_cast<LogicalOp>(parameter);
            break;
            
        // Vertex buffer methods
        case 0x1720 ... 0x175C: // Vertex buffer addresses
            {
                int idx = (method - 0x1720) / 4;
                if (parameter & 0x80000000)
                    m_vertexBufferState.address[idx] = (parameter & 0x0FFFFFFF) + m_dmaOffset[7];
                else
                    m_vertexBufferState.address[idx] = (parameter & 0x0FFFFFFF) + m_dmaOffset[6];
            }
            break;
            
        case 0x1760 ... 0x179C: // Vertex buffer formats
            {
                int idx = (method - 0x1760) / 4;
                m_vertexBufferState.type[idx] = parameter & 0xFF;
                m_vertexBufferState.stride[idx] = (parameter >> 8) & 0xFF;
                
                // Determine how many words each attribute takes
                switch (m_vertexBufferState.type[idx])
                {
                case 0x02: // none
                    m_vertexBufferState.words[idx] = 0;
                    break;
                case 0x12: // float1
                    m_vertexBufferState.words[idx] = 1;
                    break;
                case 0x16: // normpacked3
                    m_vertexBufferState.words[idx] = 1;
                    break;
                case 0x22: // float2
                    m_vertexBufferState.words[idx] = 2;
                    break;
                case 0x32: // float3
                    m_vertexBufferState.words[idx] = 3;
                    break;
                case 0x40: // d3dcolor
                    m_vertexBufferState.words[idx] = 1;
                    break;
                case 0x42: // float4
                    m_vertexBufferState.words[idx] = 4;
                    break;
                default:
                    LOG("Unsupported vertex data type %02X\n", m_vertexBufferState.type[idx]);
                    m_vertexBufferState.words[idx] = 0;
                }
                
                // Enable/disable this attribute
                if (m_vertexBufferState.words[idx] > 0)
                    m_vertexBufferState.enabled |= (1 << idx);
                else
                    m_vertexBufferState.enabled &= ~(1 << idx);
                
                // Update offsets
                m_vertexBufferState.offset[0] = 0;
                for (int n = idx + 1; n <= 16; n++) {
                    if ((m_vertexBufferState.enabled & (1 << (n - 1))) != 0)
                        m_vertexBufferState.offset[n] = m_vertexBufferState.offset[n - 1] + m_vertexBufferState.words[n - 1];
                    else
                        m_vertexBufferState.offset[n] = m_vertexBufferState.offset[n - 1];
                }
            }
            break;
            
        // Persistent vertex attributes
        case 0x1880 ... 0x18FC: {
            int v = method - 0x1880; // 16 couples, 2 float per couple
            int attr = v >> 3;
            int comp = (v >> 2) & 1;
            
            m_persistentVertexAttr.attribute[attr].iv[comp] = parameter;
            if (comp == 1) {
                m_persistentVertexAttr.attribute[attr].fv[2] = 0;
                m_persistentVertexAttr.attribute[attr].fv[3] = 1;
                
                if (attr == 0)
                    ProcessPersistentVertex();
            }
            break;
        }
            
        case 0x1900 ... 0x193C: {
            int v = method - 0x1900; // 16 dwords, 2 values per dword
            int attr = v >> 2;
            uint16_t d1 = parameter & 0xFFFF;
            uint16_t d2 = parameter >> 16;
            
            m_persistentVertexAttr.attribute[attr].fv[0] = static_cast<float>(static_cast<int16_t>(d1));
            m_persistentVertexAttr.attribute[attr].fv[1] = static_cast<float>(static_cast<int16_t>(d2));
            m_persistentVertexAttr.attribute[attr].fv[2] = 0;
            m_persistentVertexAttr.attribute[attr].fv[3] = 1;
            
            if (attr == 0)
                ProcessPersistentVertex();
            break;
        }
            
        case 0x1940 ... 0x197C: {
            int v = method - 0x1940; // 16 dwords, 4 values per dword
            int attr = v >> 2;
            uint8_t d1 = parameter & 0xFF;
            uint8_t d2 = (parameter >> 8) & 0xFF;
            uint8_t d3 = (parameter >> 16) & 0xFF;
            uint8_t d4 = parameter >> 24;
            
            // Color is ARGB
            m_persistentVertexAttr.attribute[attr].fv[0] = static_cast<float>(d1) / 255.0f;
            m_persistentVertexAttr.attribute[attr].fv[1] = static_cast<float>(d2) / 255.0f;
            m_persistentVertexAttr.attribute[attr].fv[2] = static_cast<float>(d3) / 255.0f;
            m_persistentVertexAttr.attribute[attr].fv[3] = static_cast<float>(d4) / 255.0f;
            
            if (attr == 0)
                ProcessPersistentVertex();
            break;
        }
            
        case 0x1980 ... 0x19FC: {
            int v = method - 0x1980; // 16 couples, 4 values per couple
            int attr = v >> 3;
            int comp = (v >> 1) & 3;
            uint16_t d1 = parameter & 0xFFFF;
            uint16_t d2 = parameter >> 16;
            
            m_persistentVertexAttr.attribute[attr].fv[comp] = static_cast<float>(static_cast<int16_t>(d1));
            m_persistentVertexAttr.attribute[attr].fv[comp+1] = static_cast<float>(static_cast<int16_t>(d2));
            
            if (comp == 2 && attr == 0)
                ProcessPersistentVertex();
            break;
        }
            
        case 0x1A00 ... 0x1AFC: {
            int v = method - 0x1A00; // 16 groups, 4 float per group
            int attr = v >> 4;
            int comp = (v >> 2) & 3;
            
            m_persistentVertexAttr.attribute[attr].iv[comp] = parameter;
            
            if (comp == 3 && attr == 0)
                ProcessPersistentVertex();
            break;
        }
            
        case 0x1518 ... 0x1524: {
            int v = method - 0x1518;
            int comp = v >> 2;
            
            m_persistentVertexAttr.attribute[static_cast<int>(VertexAttr::POS)].iv[comp] = parameter;
            
            if (comp == 3)
                ProcessPersistentVertex();
            break;
        }
            
        // DMA source/destination methods
        case 0x0180: // DMA notify
            ReadDMAObject(parameter, m_dmaOffset[0], m_dmaSize[0]);
            break;
            
        case 0x0184: // DMA source (surface)
            ReadDMAObject(parameter, m_dmaOffset[1], m_dmaSize[1]);
            break;
            
        case 0x0188: // DMA color (surface)
            ReadDMAObject(parameter, m_dmaOffset[2], m_dmaSize[2]);
            break;
            
        case 0x0190: // DMA source 2D
            ReadDMAObject(parameter, m_dmaOffset[3], m_dmaSize[3]);
            break;
            
        case 0x0194: // DMA source 3D
            ReadDMAObject(parameter, m_dmaOffset[4], m_dmaSize[4]);
            break;
            
        case 0x0198: // DMA color
            ReadDMAObject(parameter, m_dmaOffset[5], m_dmaSize[5]);
            break;
            
        case 0x019C: // DMA vertices
            ReadDMAObject(parameter, m_dmaOffset[6], m_dmaSize[6]);
            break;
            
        case 0x01A0: // DMA vertex buffer
            ReadDMAObject(parameter, m_dmaOffset[7], m_dmaSize[7]);
            break;
            
        case 0x01A4: // DMA indirect
            ReadDMAObject(parameter, m_dmaOffset[8], m_dmaSize[8]);
            break;
            
        case 0x01A8: // DMA swizzled texture
            ReadDMAObject(parameter, m_dmaOffset[9], m_dmaSize[9]);
            break;
            
        // Anti-aliasing and clipping
        case 0x1D7C: // Anti-aliasing control
            m_antialiasControl = parameter;
            ComputeSuperSampleFactors();
            HandleRenderTargetDimensions(m_channelState.channel[channelID].object[subchannelID].method[0x0200 / 4]);
            break;
            
        // Combiner settings
        case 0x1E60: // Combiner stages count
            m_combiner.setup.stages = parameter & 15;
            break;
            
        case 0x1E70: // Texture modes
            m_texture[0].mode = parameter & 31;
            m_texture[1].mode = (parameter >> 5) & 31;
            m_texture[2].mode = (parameter >> 10) & 31;
            m_texture[3].mode = (parameter >> 15) & 31;
            break;
            
        // Fog and misc
        case 0x02A8: // Fog color
            m_fogColor = parameter;
            break;
            
        case 0x0100: // Software method
            if (parameter != 0) {
                LOG("Software method %04X\n", parameter);
                m_pgraph[0x704 / 4] = 0x100 | (channelID << 20) | (subchannelID << 16);
                m_pgraph[0x708 / 4] = parameter;
                m_pgraph[0x100 / 4] |= 1;
                m_pgraph[0x108 / 4] |= 1;
                
                if (UpdateInterrupts()) {
                    if (m_irqCallback) {
                        m_irqCallback(1); // Signal IRQ active
                    }
                } else {
                    if (m_irqCallback) {
                        m_irqCallback(0); // Signal IRQ inactive
                    }
                }
                
                return;
            }
            break;
            
        case 0x0130: // Wait for vblank
            if (m_enableWaitVblank) {
                // Block until next vblank
                m_pullerWaiting = 1;
                return;
            }
            break;
        
        // Register combiner methods
        case 0x0288: // Final combiner input RGB mapping
            HandleFinalCombinerInputRGB(parameter);
            break;
            
        case 0x028C: // Final combiner input Alpha mapping
            HandleFinalCombinerInputAlpha(parameter);
            break;
            
        case 0x0260 ... 0x027C: // Combiner stage Alpha input mapping
            {
                int stage = (method - 0x0260) / 4;
                HandleCombinerStageAlphaInput(stage, parameter);
            }
            break;
            
        case 0x0AC0 ... 0x0ADC: // Combiner stage RGB input mapping
            {
                int stage = (method - 0x0AC0) / 4;
                HandleCombinerStageRGBInput(stage, parameter);
            }
            break;
            
        case 0x1E20: // Final combiner constant color 0
            ConvertARGB8ToFloat(parameter, m_combiner.setup.final.constantColor0);
            break;
            
        case 0x1E24: // Final combiner constant color 1
            ConvertARGB8ToFloat(parameter, m_combiner.setup.final.constantColor1);
            break;
            
        case 0x0A60 ... 0x0A7C: // Combiner stage constant color 0
            {
                int stage = (method - 0x0A60) / 4;
                ConvertARGB8ToFloat(parameter, m_combiner.setup.stage[stage].constantColor0);
            }
            break;
            
        case 0x0A80 ... 0x0A9C: // Combiner stage constant color 1
            {
                int stage = (method - 0x0A80) / 4;
                ConvertARGB8ToFloat(parameter, m_combiner.setup.stage[stage].constantColor1);
            }
            break;
            
        case 0x0AA0 ... 0x0ABC: // Combiner stage output alpha mapping
            {
                int stage = (method - 0x0AA0) / 4;
                
                m_combiner.setup.stage[stage].mapoutAlpha.cdOutput = static_cast<CombinerInputRegister>(parameter & 15);
                m_combiner.setup.stage[stage].mapoutAlpha.abOutput = static_cast<CombinerInputRegister>((parameter >> 4) & 15);
                m_combiner.setup.stage[stage].mapoutAlpha.sumOutput = static_cast<CombinerInputRegister>((parameter >> 8) & 15);
                m_combiner.setup.stage[stage].mapoutAlpha.cdDotProduct = (parameter >> 12) & 1;
                m_combiner.setup.stage[stage].mapoutAlpha.abDotProduct = (parameter >> 13) & 1;
                m_combiner.setup.stage[stage].mapoutAlpha.muxsum = (parameter >> 14) & 1;
                m_combiner.setup.stage[stage].mapoutAlpha.bias = (parameter >> 15) & 1;
                m_combiner.setup.stage[stage].mapoutAlpha.scale = (parameter >> 16) & 3;
            }
            break;
            
        case 0x1E40 ... 0x1E5C: // Combiner stage output RGB mapping
            {
                int stage = (method - 0x1E40) / 4;
                
                m_combiner.setup.stage[stage].mapoutRGB.cdOutput = static_cast<CombinerInputRegister>(parameter & 15);
                m_combiner.setup.stage[stage].mapoutRGB.abOutput = static_cast<CombinerInputRegister>((parameter >> 4) & 15);
                m_combiner.setup.stage[stage].mapoutRGB.sumOutput = static_cast<CombinerInputRegister>((parameter >> 8) & 15);
                m_combiner.setup.stage[stage].mapoutRGB.cdDotProduct = (parameter >> 12) & 1;
                m_combiner.setup.stage[stage].mapoutRGB.abDotProduct = (parameter >> 13) & 1;
                m_combiner.setup.stage[stage].mapoutRGB.muxsum = (parameter >> 14) & 1;
                m_combiner.setup.stage[stage].mapoutRGB.bias = (parameter >> 15) & 1;
                m_combiner.setup.stage[stage].mapoutRGB.scale = (parameter >> 16) & 3;
            }
            break;
            
        case 0x1D6C: // DMA indirect method offset
            // Used with 0x1D70
            break;
            
        case 0x1D70: // DMA indirect method value
            {
                // Write the value at offset [1D6C] inside DMA object [1A4]
                uint32_t offset = m_channelState.channel[channelID].object[subchannelID].method[0x1D6C / 4];
                uint32_t dmaHandle = m_channelState.channel[channelID].object[subchannelID].method[0x1A4 / 4];
                uint32_t dmaOffset, dmaSize;
                
                ReadDMAObject(dmaHandle, dmaOffset, dmaSize);
                
                // Write the value to memory
                uint32_t* dest = reinterpret_cast<uint32_t*>(DirectAccessPtr(dmaOffset + offset));
                if (dest) {
                    *dest = parameter;
                }
                
                // Software expects to find the parameter at PGRAPH offset b10
                m_pgraph[0xB10 / 4] = parameter << 2;
            }
            break;
            
        // Other methods
        default:
            //LOG("Unhandled graphics method: 0x%04X = 0x%08X\n", method, parameter);
            break;
    }
}

void GeForce3::ExecuteMethodM2MF(uint32_t channelID, uint32_t subchannelID, uint32_t method, uint32_t parameter)
{
    switch (method) {
        case 0x0180: // Notify
            ReadDMAObject(parameter, m_dmaOffset[10], m_dmaSize[10]);
            break;
            
        default:
            //LOG("Unhandled M2MF method: 0x%04X = 0x%08X\n", method, parameter);
            break;
    }
}

void GeForce3::ExecuteMethodSurf2D(uint32_t channelID, uint32_t subchannelID, uint32_t method, uint32_t parameter)
{
    switch (method) {
        case 0x0184: // Source
            ReadDMAObject(parameter, m_dmaOffset[11], m_dmaSize[11]);
            break;
            
        case 0x0188: // Destination
            ReadDMAObject(parameter, m_dmaOffset[12], m_dmaSize[12]);
            break;
            
        case 0x0300: // Format
            m_bitBlit.format = parameter; // 0xA is a8r8g8b8
            break;
            
        case 0x0304: // Pitch
            m_bitBlit.sourcePitch = parameter & 0xFFFF;
            m_bitBlit.destinationPitch = parameter >> 16;
            break;
            
        case 0x0308: // Source address
            m_bitBlit.sourceAddress = m_dmaOffset[11] + parameter;
            break;
            
        case 0x030C: // Destination address
            m_bitBlit.destinationAddress = m_dmaOffset[12] + parameter;
            break;
            
        default:
            //LOG("Unhandled Surf2D method: 0x%04X = 0x%08X\n", method, parameter);
            break;
    }
}

void GeForce3::ExecuteMethodBlit(uint32_t channelID, uint32_t subchannelID, uint32_t method, uint32_t parameter)
{
    switch (method) {
        case 0x019C: // Surface object handle
            break;
            
        case 0x02FC: // Operation
            m_bitBlit.op = parameter;
            break;
            
        case 0x0300: // Source XY
            m_bitBlit.sourceX = parameter & 0xFFFF;
            m_bitBlit.sourceY = parameter >> 16;
            break;
            
        case 0x0304: // Destination XY
            m_bitBlit.destX = parameter & 0xFFFF;
            m_bitBlit.destY = parameter >> 16;
            break;
            
        case 0x0308: // Size
            m_bitBlit.width = parameter & 0xFFFF;
            m_bitBlit.height = parameter >> 16;
            PerformBlit();
            break;
            
        default:
            //LOG("Unhandled Blit method: 0x%04X = 0x%08X\n", method, parameter);
            break;
    }
}

void GeForce3::HandleTextureMethod(uint32_t method, uint32_t parameter)
{
    // Calculate the texture unit (0-3) and the method within that unit
    int texUnit = ((method - 0x1B00) >> 6) & 3;
    uint32_t texMethod = method & ~0xC0;
    
    switch (texMethod) {
        case 0x1B00: // Texture buffer offset
            {
                uint32_t offset = parameter;
                m_texture[texUnit].buffer = DirectAccessPtr(offset);
            }
            break;
            
        case 0x1B04: // Texture format
            {
                m_texture[texUnit].dma0 = (parameter >> 0) & 1;
                m_texture[texUnit].dma1 = (parameter >> 1) & 1;
                m_texture[texUnit].cubic = (parameter >> 2) & 1;
                m_texture[texUnit].noBorder = (parameter >> 3) & 1;
                m_texture[texUnit].dims = (parameter >> 4) & 15;
                m_texture[texUnit].mipmap = (parameter >> 19) & 1;
                
                int format = (parameter >> 8) & 0xFF;
                int baseSizeU = (parameter >> 20) & 15;
                int baseSizeV = (parameter >> 24) & 15;
                int baseSizeW = (parameter >> 28) & 15;
                
                m_texture[texUnit].sizeS = 1 << baseSizeU;
                m_texture[texUnit].sizeT = 1 << baseSizeV;
                m_texture[texUnit].sizeR = 1 << baseSizeW;
                m_texture[texUnit].format = static_cast<TexFormat>(format);
                
                // Check if the texture format is rectangular
                bool isRectangular = false;
                switch (m_texture[texUnit].format) {
                    case TexFormat::A1R5G5B5_RECT:
                    case TexFormat::R5G6B5_RECT:
                    case TexFormat::A8R8G8B8_RECT:
                    case TexFormat::L8_RECT:
                    case TexFormat::DSDT8_RECT:
                    case TexFormat::A4R4G4B4_RECT:
                    case TexFormat::R8G8B8_RECT:
                    case TexFormat::A8L8_RECT:
                    case TexFormat::Z24_RECT:
                    case TexFormat::Z16_RECT:
                    case TexFormat::HILO16_RECT:
                    case TexFormat::SIGNED_HILO8_RECT:
                        isRectangular = true;
                        break;
                    default:
                        isRectangular = false;
                }
                m_texture[texUnit].rectangular = isRectangular;
            }
            break;
            
        case 0x1B08: // Texture addressing modes
            m_texture[texUnit].addrModeS = (parameter >> 0) & 15;
            m_texture[texUnit].addrModeT = (parameter >> 8) & 15;
            m_texture[texUnit].addrModeR = (parameter >> 16) & 15;
            break;
            
        case 0x1B0C: // Texture control
            {
                m_texture[texUnit].colorKey = (parameter >> 0) & 3;
                m_texture[texUnit].imageField = (parameter >> 3) & 1;
                m_texture[texUnit].aniso = (parameter >> 4) & 3;
                m_texture[texUnit].mipMapMaxLOD = (parameter >> 6) & 0xFFF;
                m_texture[texUnit].mipMapMinLOD = (parameter >> 18) & 0xFFF;
                m_texture[texUnit].enabled = (parameter >> 30) & 3;
            }
            break;
            
        case 0x1B10: // Texture filter
            {
                m_texture[texUnit].rectanglePitch = parameter >> 16;
                // Enable bilinear filtering
                m_bilinearFilter = (parameter != 0);
            }
            break;
            
        case 0x1B14: // Texture filter
            break;
            
        case 0x1B1C: // Texture dimensions for rectangular textures
            m_texture[texUnit].rectHeight = parameter & 0xFFFF;
            m_texture[texUnit].rectWidth = parameter >> 16;
            break;
    }
}

void GeForce3::HandleDrawCommand(uint32_t parameter)
{
    m_vertexCount = 0;
    m_vertexFirst = 0;
    m_vertexAccumulated = 0;
    m_indexesLeftCount = 0;
    m_indexesLeftFirst = 0;
    m_primitivesCount = 0;
    m_primitiveType = static_cast<PrimitiveType>(parameter);
    
    if (parameter == 0) { // End primitive
        m_primitivesBatchCount++;
    } else {
        // Select the appropriate render callback
        if (m_combinerEnabled) {
            m_renderCallback = &GeForce3::RenderRegisterCombiners;
        } else if (m_texture[0].enabled) {
            m_renderCallback = &GeForce3::RenderTextureSimple;
        } else {
            m_renderCallback = &GeForce3::RenderColor;
        }
    }
}

void GeForce3::HandleDrawIndices(int multiply, uint32_t parameter)
{
    // Read indices from memory
    int n = m_indexesLeftFirst + m_indexesLeftCount;
    
    if (multiply == 2) {
        // 16-bit indices (two per DWORD)
        m_vertexIndices[n & 1023] = parameter & 0xFFFF;
        m_vertexIndices[(n + 1) & 1023] = (parameter >> 16) & 0xFFFF;
        m_indexesLeftCount += 2;
    } else {
        // 32-bit indices (one per DWORD)
        m_vertexIndices[n & 1023] = parameter;
        m_indexesLeftCount += 1;
    }
    
    // Read vertices based on indices
    ReadVerticesFromIndices(m_vertexFirst, parameter, multiply);
    AssemblePrimitive(m_vertexFirst, multiply);
    m_vertexFirst = (m_vertexFirst + multiply) & 1023;
}

void GeForce3::HandleDrawVerticesWithOffset(uint32_t parameter)
{
    int offset = parameter & 0xFFFFFF;
    int count = (parameter >> 24) & 0xFF;
    
    ReadVerticesWithOffset(m_vertexFirst, offset, count + 1);
    AssemblePrimitive(m_vertexFirst, count + 1);
    m_vertexFirst = (m_vertexFirst + count + 1) & 1023;
}

int GeForce3::HandleDrawRawVertices(uint32_t parameter)
{
    int count = ReadRawVertices(m_vertexFirst, parameter, 1);
    AssemblePrimitive(m_vertexFirst, 1);
    m_vertexFirst = (m_vertexFirst + 1) & 1023;
    return count;
}

void GeForce3::HandleRenderTargetDimensions(uint32_t parameter)
{
    int x = parameter & 0xFFFF;
    int w = (parameter >> 16) & 0xFFFF;
    x = static_cast<int>(x * m_supersampleFactorX);
    w = static_cast<int>(w * m_supersampleFactorX);
    m_renderTargetLimits.setx(x, x + w - 1);
}

void GeForce3::HandleRenderTargetHeight(uint32_t parameter)
{
    int y = parameter & 0xFFFF;
    int h = (parameter >> 16) & 0xFFFF;
    y = static_cast<int>(y * m_supersampleFactorY);
    h = static_cast<int>(h * m_supersampleFactorY);
    m_renderTargetLimits.sety(y, y + h - 1);
}

void GeForce3::HandleRenderTargetFormat(uint32_t parameter)
{
    m_log2HeightRenderTarget = (parameter >> 24) & 0xFF;
    m_log2WidthRenderTarget = (parameter >> 16) & 0xFF;
    m_antialiasingRenderTarget = (parameter >> 12) & 0xF;
    m_renderTargetType = static_cast<RenderTargetType>((parameter >> 8) & 0xF);
    m_depthFormat = static_cast<DepthFormat>((parameter >> 4) & 0xF);
    m_colorFormat = static_cast<ColorFormat>((parameter >> 0) & 0xF);
    
    ComputeSuperSampleFactors();
    HandleRenderTargetDimensions(m_channelState.channel[0].object[0].method[0x0200 / 4]);
    UpdateRenderTargetSize();
    
    // Determine bytes per pixel based on color format
    switch (m_colorFormat) {
        case ColorFormat::R5G6B5:
            m_bytesPerPixel = 2;
            break;
        case ColorFormat::X8R8G8B8_Z8R8G8B8:
        case ColorFormat::X8R8G8B8_X8R8G8B8:
        case ColorFormat::A8R8G8B8:
            m_bytesPerPixel = 4;
            break;
        case ColorFormat::B8:
            m_bytesPerPixel = 1;
            break;
        default:
            m_bytesPerPixel = 4;
            break;
    }
}

void GeForce3::HandleFinalCombinerInputRGB(uint32_t parameter)
{
    m_combiner.setup.final.mapinRGB.dInput = static_cast<CombinerInputRegister>(parameter & 15);
    m_combiner.setup.final.mapinRGB.dComponent = (parameter >> 4) & 1;
    m_combiner.setup.final.mapinRGB.dMapping = static_cast<CombinerMapFunction>((parameter >> 5) & 7);
    
    m_combiner.setup.final.mapinRGB.cInput = static_cast<CombinerInputRegister>((parameter >> 8) & 15);
    m_combiner.setup.final.mapinRGB.cComponent = (parameter >> 12) & 1;
    m_combiner.setup.final.mapinRGB.cMapping = static_cast<CombinerMapFunction>((parameter >> 13) & 7);
    
    m_combiner.setup.final.mapinRGB.bInput = static_cast<CombinerInputRegister>((parameter >> 16) & 15);
    m_combiner.setup.final.mapinRGB.bComponent = (parameter >> 20) & 1;
    m_combiner.setup.final.mapinRGB.bMapping = static_cast<CombinerMapFunction>((parameter >> 21) & 7);
    
    m_combiner.setup.final.mapinRGB.aInput = static_cast<CombinerInputRegister>((parameter >> 24) & 15);
    m_combiner.setup.final.mapinRGB.aComponent = (parameter >> 28) & 1;
    m_combiner.setup.final.mapinRGB.aMapping = static_cast<CombinerMapFunction>((parameter >> 29) & 7);
}

void GeForce3::HandleFinalCombinerInputAlpha(uint32_t parameter)
{
    m_combiner.setup.final.colorSumClamp = (parameter >> 7) & 1;
    
    m_combiner.setup.final.mapinAlpha.gInput = static_cast<CombinerInputRegister>((parameter >> 8) & 15);
    m_combiner.setup.final.mapinAlpha.gComponent = (parameter >> 12) & 1;
    m_combiner.setup.final.mapinAlpha.gMapping = static_cast<CombinerMapFunction>((parameter >> 13) & 7);
    
    m_combiner.setup.final.mapinRGB.fInput = static_cast<CombinerInputRegister>((parameter >> 16) & 15);
    m_combiner.setup.final.mapinRGB.fComponent = (parameter >> 20) & 1;
    m_combiner.setup.final.mapinRGB.fMapping = static_cast<CombinerMapFunction>((parameter >> 21) & 7);
    
    m_combiner.setup.final.mapinRGB.eInput = static_cast<CombinerInputRegister>((parameter >> 24) & 15);
    m_combiner.setup.final.mapinRGB.eComponent = (parameter >> 28) & 1;
    m_combiner.setup.final.mapinRGB.eMapping = static_cast<CombinerMapFunction>((parameter >> 29) & 7);
}

void GeForce3::HandleCombinerStageAlphaInput(int stage, uint32_t parameter)
{
    m_combiner.setup.stage[stage].mapinAlpha.dInput = static_cast<CombinerInputRegister>(parameter & 15);
    m_combiner.setup.stage[stage].mapinAlpha.dComponent = (parameter >> 4) & 1;
    m_combiner.setup.stage[stage].mapinAlpha.dMapping = static_cast<CombinerMapFunction>((parameter >> 5) & 7);
    
    m_combiner.setup.stage[stage].mapinAlpha.cInput = static_cast<CombinerInputRegister>((parameter >> 8) & 15);
    m_combiner.setup.stage[stage].mapinAlpha.cComponent = (parameter >> 12) & 1;
    m_combiner.setup.stage[stage].mapinAlpha.cMapping = static_cast<CombinerMapFunction>((parameter >> 13) & 7);
    
    m_combiner.setup.stage[stage].mapinAlpha.bInput = static_cast<CombinerInputRegister>((parameter >> 16) & 15);
    m_combiner.setup.stage[stage].mapinAlpha.bComponent = (parameter >> 20) & 1;
    m_combiner.setup.stage[stage].mapinAlpha.bMapping = static_cast<CombinerMapFunction>((parameter >> 21) & 7);
    
    m_combiner.setup.stage[stage].mapinAlpha.aInput = static_cast<CombinerInputRegister>((parameter >> 24) & 15);
    m_combiner.setup.stage[stage].mapinAlpha.aComponent = (parameter >> 28) & 1;
    m_combiner.setup.stage[stage].mapinAlpha.aMapping = static_cast<CombinerMapFunction>((parameter >> 29) & 7);
}

void GeForce3::HandleCombinerStageRGBInput(int stage, uint32_t parameter)
{
    m_combiner.setup.stage[stage].mapinRGB.dInput = static_cast<CombinerInputRegister>(parameter & 15);
    m_combiner.setup.stage[stage].mapinRGB.dComponent = (parameter >> 4) & 1;
    m_combiner.setup.stage[stage].mapinRGB.dMapping = static_cast<CombinerMapFunction>((parameter >> 5) & 7);
    
    m_combiner.setup.stage[stage].mapinRGB.cInput = static_cast<CombinerInputRegister>((parameter >> 8) & 15);
    m_combiner.setup.stage[stage].mapinRGB.cComponent = (parameter >> 12) & 1;
    m_combiner.setup.stage[stage].mapinRGB.cMapping = static_cast<CombinerMapFunction>((parameter >> 13) & 7);
    
    m_combiner.setup.stage[stage].mapinRGB.bInput = static_cast<CombinerInputRegister>((parameter >> 16) & 15);
    m_combiner.setup.stage[stage].mapinRGB.bComponent = (parameter >> 20) & 1;
    m_combiner.setup.stage[stage].mapinRGB.bMapping = static_cast<CombinerMapFunction>((parameter >> 21) & 7);
    
    m_combiner.setup.stage[stage].mapinRGB.aInput = static_cast<CombinerInputRegister>((parameter >> 24) & 15);
    m_combiner.setup.stage[stage].mapinRGB.aComponent = (parameter >> 28) & 1;
    m_combiner.setup.stage[stage].mapinRGB.aMapping = static_cast<CombinerMapFunction>((parameter >> 29) & 7);
}

void GeForce3::clear_render_target(int what, uint32_t value)
{
    // Don't do anything if nothing to clear
    if (what == 0)
        return;
    
    if (!m_renderTarget)
        return;
        
    int width = m_renderTargetLimits.right() - m_renderTargetLimits.left() + 1;
    int height = m_renderTargetLimits.bottom() - m_renderTargetLimits.top() + 1;
    
    // Clear RGB channels?
    if (what & (1 << 4)) {
        // For each scanline
        for (int y = m_clearRect.top(); y <= m_clearRect.bottom(); y++) {
            uint32_t* dest = m_renderTarget + y * (m_renderTargetPitch / 4) + m_clearRect.left();
            
            // For each pixel
            for (int x = 0; x < (m_clearRect.right() - m_clearRect.left() + 1); x++) {
                *dest++ = value;
            }
        }
    }
    
    LOG("Clear color buffer: %08X\n", value);
}

void GeForce3::clear_depth_buffer(int what, uint32_t value)
{
    // Don't do anything if nothing to clear
    if (what == 0)
        return;
    
    if (!m_depthBuffer)
        return;
        
    // For each scanline
    for (int y = m_clearRect.top(); y <= m_clearRect.bottom(); y++) {
        if (m_depthFormat == DepthFormat::Z24S8) {
            uint32_t* dest = m_depthBuffer + y * (m_depthBufferPitch / 4) + m_clearRect.left();
            
            // For each pixel
            for (int x = 0; x < (m_clearRect.right() - m_clearRect.left() + 1); x++) {
                *dest++ = value;
            }
        } else {
            uint16_t* dest = reinterpret_cast<uint16_t*>(m_depthBuffer) + y * (m_depthBufferPitch / 2) + m_clearRect.left();
            uint16_t depth16 = static_cast<uint16_t>(value >> 8);
            
            // For each pixel
            for (int x = 0; x < (m_clearRect.right() - m_clearRect.left() + 1); x++) {
                *dest++ = depth16;
            }
        }
    }
    
    LOG("Clear depth buffer: %08X\n", value);
}

void GeForce3::ProcessPersistentVertex()
{
    // Transform coordinates and render a single point
    NV2AVertex_t v;
    
    // Use position from persistent vertex attribute 0
    ConvertVertices(&m_persistentVertexAttr, &v);
    
    // For test purposes, let's just draw a single pixel
    int x = static_cast<int>(v.x);
    int y = static_cast<int>(v.y);
    
    if (x >= 0 && x < m_renderTargetLimits.right() && y >= 0 && y < m_renderTargetLimits.bottom()) {
        uint32_t color = 0;
        
        // Extract color from persistent vertex attribute 3 (COLOR0)
        float r = m_persistentVertexAttr.attribute[static_cast<int>(VertexAttr::COLOR0)].fv[0];
        float g = m_persistentVertexAttr.attribute[static_cast<int>(VertexAttr::COLOR0)].fv[1];
        float b = m_persistentVertexAttr.attribute[static_cast<int>(VertexAttr::COLOR0)].fv[2];
        float a = m_persistentVertexAttr.attribute[static_cast<int>(VertexAttr::COLOR0)].fv[3];
        
        color = (static_cast<uint32_t>(a * 255.0f) << 24) |
                (static_cast<uint32_t>(r * 255.0f) << 16) |
                (static_cast<uint32_t>(g * 255.0f) << 8) |
                 static_cast<uint32_t>(b * 255.0f);
                 
        WritePixel(x, y, color, static_cast<int>(v.p[static_cast<int>(VertexParameter::PARAM_Z)] * 0xFFFFFF));
    }
}

// Vertex processor implementation
void GeForce3::DecodeVertexInstruction(int address)
{
    auto& ins = m_vertexProgram.instructions[address];
    auto& decoded = ins.decoded;
    
    decoded.NegateA = ins.data[1] & (1 << 8);
    decoded.ParameterTypeA = (ins.data[2] >> 26) & 3;
    decoded.TempIndexA = (ins.data[2] >> 28) & 15;
    decoded.SwizzleA[0] = (ins.data[1] >> 6) & 3;
    decoded.SwizzleA[1] = (ins.data[1] >> 4) & 3;
    decoded.SwizzleA[2] = (ins.data[1] >> 2) & 3;
    decoded.SwizzleA[3] = (ins.data[1] >> 0) & 3;
    
    decoded.NegateB = ins.data[2] & (1 << 25);
    decoded.ParameterTypeB = (ins.data[2] >> 11) & 3;
    decoded.TempIndexB = (ins.data[2] >> 13) & 15;
    decoded.SwizzleB[0] = (ins.data[2] >> 23) & 3;
    decoded.SwizzleB[1] = (ins.data[2] >> 21) & 3;
    decoded.SwizzleB[2] = (ins.data[2] >> 19) & 3;
    decoded.SwizzleB[3] = (ins.data[2] >> 17) & 3;
    
    decoded.NegateC = ins.data[2] & (1 << 10);
    decoded.ParameterTypeC = (ins.data[3] >> 28) & 3;
    decoded.TempIndexC = ((ins.data[2] & 3) << 2) + (ins.data[3] >> 30);
    decoded.SwizzleC[0] = (ins.data[2] >> 8) & 3;
    decoded.SwizzleC[1] = (ins.data[2] >> 6) & 3;
    decoded.SwizzleC[2] = (ins.data[2] >> 4) & 3;
    decoded.SwizzleC[3] = (ins.data[2] >> 2) & 3;
    
    decoded.VecOperation = static_cast<VectorOp>((ins.data[1] >> 21) & 15);
    decoded.ScaOperation = static_cast<ScalarOp>((ins.data[1] >> 25) & 15);
    
    decoded.OutputWriteMask = ((ins.data[3] >> 12) & 15);
    decoded.MultiplexerControl = ins.data[3] & 4;
    decoded.VecTempIndex = (ins.data[3] >> 20) & 15;
    decoded.OutputIndex = (ins.data[3] >> 3) & 255;
    decoded.OutputSelect = ins.data[3] & 0x800;
    decoded.VecTempWriteMask = (ins.data[3] >> 24) & 15;
    decoded.ScaTempWriteMask = (ins.data[3] >> 16) & 15;
    decoded.InputIndex = (ins.data[1] >> 9) & 15;
    decoded.SourceConstantIndex = (ins.data[1] >> 13) & 255;
    decoded.Usea0x = ins.data[3] & 2;
    decoded.EndOfProgram = ins.data[3] & 1;
}

void GeForce3::ExecuteVertexProgram()
{
    // Initialize the vertex program
    m_vertexProgram.ip = m_vertexProgram.startInstruction;
    m_vertexProgram.a0x = 0;
    
    // Execute instructions until we reach the end of the program
    bool endOfProgram = false;
    while (!endOfProgram) {
        endOfProgram = ExecuteVertexInstruction();
        m_vertexProgram.ip++;
    }
}

bool GeForce3::ExecuteVertexInstruction()
{
    auto& ins = m_vertexProgram.instructions[m_vertexProgram.ip].decoded;
    
    // Prepare inputs 
    float inputA[4] = {0, 0, 0, 0};
    float inputB[4] = {0, 0, 0, 0};
    float inputC[4] = {0, 0, 0, 0};
    float outputVec[4] = {0, 0, 0, 0};
    float outputSca[4] = {0, 0, 0, 0};
    
    // Generate inputs
    GenerateInput(inputA, ins.NegateA, ins.ParameterTypeA, ins.TempIndexA, ins.SwizzleA);
    GenerateInput(inputB, ins.NegateB, ins.ParameterTypeB, ins.TempIndexB, ins.SwizzleB);
    GenerateInput(inputC, ins.NegateC, ins.ParameterTypeC, ins.TempIndexC, ins.SwizzleC);
    
    // Execute vector operation
    ComputeVectorOperation(outputVec, ins.VecOperation, inputA, inputB, inputC);
    
    // Execute scalar operation
    ComputeScalarOperation(outputSca, ins.ScaOperation, inputA, inputB, inputC);
    
    // Assign destinations
    if (ins.VecOperation != VectorOp::NOP) {
        if (ins.VecOperation == VectorOp::ARL) {
            // Address register load
            m_vertexProgram.a0x = static_cast<int>(outputVec[0]);
        } else {
            // Assign to temporary register
            if (ins.VecTempWriteMask != 0) {
                AssignToRegister(ins.VecTempIndex, outputVec, ins.VecTempWriteMask);
            }
            
            // Assign to output register
            if (ins.OutputWriteMask != 0 && ins.MultiplexerControl == 0) {
                if (ins.OutputSelect != 0) {
                    // Assign to vertex output
                    AssignToOutput(ins.OutputIndex, outputVec, ins.OutputWriteMask);
                    
                    // If this is the position output, also update r12
                    if (ins.OutputIndex == 0) {
                        for (int i = 0; i < 4; i++) {
                            m_vertexProgram.registers[12].fv[i] = m_vertexOutput->attribute[ins.OutputIndex].fv[i];
                        }
                    }
                } else {
                    // Assign to constant register
                    AssignToConstant(ins.OutputIndex, outputVec, ins.OutputWriteMask);
                }
            }
        }
    }
    
    if (ins.ScaOperation != ScalarOp::NOP) {
        // Assign to temporary register
        if (ins.ScaTempWriteMask != 0) {
            int regIndex = (ins.VecOperation != VectorOp::NOP) ? 1 : ins.VecTempIndex;
            AssignToRegister(regIndex, outputSca, ins.ScaTempWriteMask);
        }
        
        // Assign to output register
        if (ins.OutputWriteMask != 0 && ins.MultiplexerControl != 0) {
            AssignToOutput(ins.OutputIndex, outputSca, ins.OutputWriteMask);
            
            // If this is the position output, also update r12
            if (ins.OutputIndex == 0) {
                for (int i = 0; i < 4; i++) {
                    m_vertexProgram.registers[12].fv[i] = m_vertexOutput->attribute[ins.OutputIndex].fv[i];
                }
            }
        }
    }
    
    return ins.EndOfProgram != 0;
}

void GeForce3::GenerateInput(float* output, bool negate, int type, int tempIndex, const int* swizzle)
{
    // Select source based on type
    float* source = nullptr;
    switch (type) {
        case 1: // Register (Rn)
            source = m_vertexProgram.registers[tempIndex].fv;
            break;
            
        case 2: // Vertex input (Vn)
            source = m_vertexInput->attribute[m_vertexProgram.instructions[m_vertexProgram.ip].decoded.InputIndex].fv;
            break;
            
        case 3: // Constant (Cn)
            {
                int sourceConstantIndex = m_vertexProgram.instructions[m_vertexProgram.ip].decoded.SourceConstantIndex;
                if (m_vertexProgram.instructions[m_vertexProgram.ip].decoded.Usea0x) {
                    sourceConstantIndex += m_vertexProgram.a0x;
                }
                source = m_vertexProgram.constants[sourceConstantIndex].fv;
            }
            break;
            
        default:
            // Zero
            output[0] = output[1] = output[2] = output[3] = 0.0f;
            return;
    }
    
    // Apply swizzling
    for (int i = 0; i < 4; i++) {
        output[i] = source[swizzle[i]];
    }
    
    // Apply negation if requested
    if (negate) {
        for (int i = 0; i < 4; i++) {
            output[i] = -output[i];
        }
    }
}

void GeForce3::ComputeVectorOperation(float* output, VectorOp op, const float* a, const float* b, const float* c)
{
    switch (op) {
        case VectorOp::NOP:
            // No operation
            break;
            
        case VectorOp::MOV:
            // Move
            output[0] = a[0];
            output[1] = a[1];
            output[2] = a[2];
            output[3] = a[3];
            break;
            
        case VectorOp::MUL:
            // Multiply
            output[0] = a[0] * b[0];
            output[1] = a[1] * b[1];
            output[2] = a[2] * b[2];
            output[3] = a[3] * b[3];
            break;
            
        case VectorOp::ADD:
            // Add
            output[0] = a[0] + c[0];
            output[1] = a[1] + c[1];
            output[2] = a[2] + c[2];
            output[3] = a[3] + c[3];
            break;
            
        case VectorOp::MAD:
            // Multiply and Add
            output[0] = a[0] * b[0] + c[0];
            output[1] = a[1] * b[1] + c[1];
            output[2] = a[2] * b[2] + c[2];
            output[3] = a[3] * b[3] + c[3];
            break;
            
        case VectorOp::DP3:
            // 3-component Dot Product
            {
                float dp = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
                output[0] = output[1] = output[2] = output[3] = dp;
            }
            break;
            
        case VectorOp::DPH:
            // Homogeneous Dot Product
            {
                float dp = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + b[3];
                output[0] = output[1] = output[2] = output[3] = dp;
            }
            break;
            
        case VectorOp::DP4:
            // 4-component Dot Product
            {
                float dp = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
                output[0] = output[1] = output[2] = output[3] = dp;
            }
            break;
            
        case VectorOp::DST:
            // Distance Vector
            output[0] = 1.0f;
            output[1] = a[1] * b[1];
            output[2] = a[2];
            output[3] = b[3];
            break;
            
        case VectorOp::MIN:
            // Minimum
            output[0] = std::min(a[0], b[0]);
            output[1] = std::min(a[1], b[1]);
            output[2] = std::min(a[2], b[2]);
            output[3] = std::min(a[3], b[3]);
            break;
            
        case VectorOp::MAX:
            // Maximum
            output[0] = std::max(a[0], b[0]);
            output[1] = std::max(a[1], b[1]);
            output[2] = std::max(a[2], b[2]);
            output[3] = std::max(a[3], b[3]);
            break;
            
        case VectorOp::SLT:
            // Set Less Than
            output[0] = (a[0] < b[0]) ? 1.0f : 0.0f;
            output[1] = (a[1] < b[1]) ? 1.0f : 0.0f;
            output[2] = (a[2] < b[2]) ? 1.0f : 0.0f;
            output[3] = (a[3] < b[3]) ? 1.0f : 0.0f;
            break;
            
        case VectorOp::SGE:
            // Set Greater or Equal
            output[0] = (a[0] >= b[0]) ? 1.0f : 0.0f;
            output[1] = (a[1] >= b[1]) ? 1.0f : 0.0f;
            output[2] = (a[2] >= b[2]) ? 1.0f : 0.0f;
            output[3] = (a[3] >= b[3]) ? 1.0f : 0.0f;
            break;
            
        case VectorOp::ARL:
            // Address Register Load
            output[0] = floor(a[0]);
            break;
    }
}

void GeForce3::ComputeScalarOperation(float* output, ScalarOp op, const float* a, const float* b, const float* c)
{
    union {
        float f;
        uint32_t i;
    } t;
    int e;
    
    switch (op) {
        case ScalarOp::NOP:
            // No operation
            break;
            
        case ScalarOp::IMV:
            // Identity Move
            output[0] = c[0];
            output[1] = c[1];
            output[2] = c[2];
            output[3] = c[3];
            break;
            
        case ScalarOp::RCP:
            // Reciprocal
            if (c[0] == 0) {
                t.f = std::numeric_limits<float>::infinity();
            } else if (c[0] == 1.0f) {
                t.f = 1.0f;
            } else {
                t.f = 1.0f / c[0];
            }
            output[0] = output[1] = output[2] = output[3] = t.f;
            break;
            
        case ScalarOp::RCC:
            // Range-Clamped Reciprocal
            {
                t.f = c[0];
                if ((t.f < 0) && (t.f > -5.42101e-20f)) {
                    t.f = -5.42101e-20f;
                } else if ((t.f >= 0) && (t.f < 5.42101e-20f)) {
                    t.f = 5.42101e-20f;
                }
                if (t.f != 1.0f) {
                    t.f = 1.0f / t.f;
                }
                output[0] = output[1] = output[2] = output[3] = t.f;
            }
            break;
            
        case ScalarOp::RSQ:
            // Reciprocal Square Root
            output[0] = output[1] = output[2] = output[3] = 1.0f / sqrt(std::abs(c[0]));
            break;
            
        case ScalarOp::EXP:
            // Exponential Base 2
            {
                output[0] = pow(2, floor(c[0]));
                output[1] = c[0] - floor(c[0]);
                t.f = pow(2, c[0]);
                t.i = t.i & 0xffffff00;
                output[2] = t.f;
                output[3] = 1.0f;
            }
            break;
            
        case ScalarOp::LOG:
            // Logarithm Base 2
            {
                output[1] = frexp(c[0], &e) * 2.0f; // frexp gives mantissa as 0.5...1
                output[0] = e - 1;
                t.f = log2(std::abs(c[0]));
                t.i = t.i & 0xffffff00;
                output[2] = t.f;
                output[3] = 1.0f;
            }
            break;
            
        case ScalarOp::LIT:
            // Light Coefficients
            {
                output[0] = 1.0f;
                output[1] = std::max(0.0f, std::min(c[0], 1.0f));
                output[2] = c[0] > 0 ? pow(std::max(c[1], 0.0f), c[3]) : 0.0f;
                output[3] = 1.0f;
            }
            break;
    }
}

void GeForce3::AssignToRegister(int index, const float* value, int mask)
{
    for (int i = 0; i < 4; i++) {
        if (mask & (8 >> i)) {
            m_vertexProgram.registers[index].fv[i] = value[i];
        }
    }
}

void GeForce3::AssignToOutput(int index, const float* value, int mask)
{
    for (int i = 0; i < 4; i++) {
        if (mask & (8 >> i)) {
            m_vertexOutput->attribute[index].fv[i] = value[i];
        }
    }
}

void GeForce3::AssignToConstant(int index, const float* value, int mask)
{
    for (int i = 0; i < 4; i++) {
        if (mask & (8 >> i)) {
            m_vertexProgram.constants[index].fv[i] = value[i];
        }
    }
}

// Add this method to convert a swizzled texture to a linear texture
void GeForce3::PerformBlit()
{
    // Check format is supported
    if (m_bitBlit.format != 0xA) {
        LOG("Unsupported blit format %d\n", m_bitBlit.format);
        return;
    }
    
    // Get pointers to source and destination
    uint32_t* srcRow = (uint32_t*)DirectAccessPtr(m_bitBlit.sourceAddress + 
                                                 m_bitBlit.sourcePitch * m_bitBlit.sourceY + 
                                                 m_bitBlit.sourceX * 4);
    uint32_t* destRow = (uint32_t*)DirectAccessPtr(m_bitBlit.destinationAddress + 
                                                  m_bitBlit.destinationPitch * m_bitBlit.destY + 
                                                  m_bitBlit.destX * 4);
    
    // Perform the blit operation
    for (int y = 0; y < m_bitBlit.height; y++) {
        uint32_t* src = srcRow;
        uint32_t* dest = destRow;
        
        for (int x = 0; x < m_bitBlit.width; x++) {
            *dest = *src;
            dest++;
            src++;
        }
        
        srcRow += m_bitBlit.sourcePitch / 4;
        destRow += m_bitBlit.destinationPitch / 4;
    }
}

// Vertex and rendering implementation
void GeForce3::AssemblePrimitive(int source, int count)
{
    uint32_t primitivesCount = m_primitivesCount;
    
    for (; count > 0; count--) {
        VertexNV* v = &m_vertexSoftware[source];
        
        if (m_primitiveType == PrimitiveType::QUADS) {
            ConvertVertices(v, m_vertexXY + ((m_vertexCount + m_vertexAccumulated) & 1023));
            m_vertexAccumulated++;
            
            if (m_vertexAccumulated == 4) {
                m_primitivesCount++;
                m_vertexAccumulated = 0;
                
                RenderTriangleClipping(m_renderTargetLimits, 
                                     m_vertexXY[m_vertexCount], 
                                     m_vertexXY[m_vertexCount + 1], 
                                     m_vertexXY[m_vertexCount + 2]);
                                     
                RenderTriangleClipping(m_renderTargetLimits, 
                                     m_vertexXY[m_vertexCount], 
                                     m_vertexXY[m_vertexCount + 2], 
                                     m_vertexXY[m_vertexCount + 3]);
                                     
                m_vertexCount = (m_vertexCount + 4) & 1023;
                m_rasterizer->Wait();
            }
        }
        else if (m_primitiveType == PrimitiveType::TRIANGLES) {
            ConvertVertices(v, m_vertexXY + ((m_vertexCount + m_vertexAccumulated) & 1023));
            m_vertexAccumulated++;
            
            if (m_vertexAccumulated == 3) {
                m_primitivesCount++;
                m_vertexAccumulated = 0;
                
                RenderTriangleClipping(m_renderTargetLimits, 
                                     m_vertexXY[m_vertexCount], 
                                     m_vertexXY[(m_vertexCount + 1) & 1023], 
                                     m_vertexXY[(m_vertexCount + 2) & 1023]);
                                     
                m_vertexCount = (m_vertexCount + 3) & 1023;
                m_rasterizer->Wait();
            }
        }
        else if (m_primitiveType == PrimitiveType::TRIANGLE_FAN) {
            if (m_vertexAccumulated == 0) {
                ConvertVertices(v, m_vertexXY + 1024);
                m_vertexAccumulated = 1;
            }
            else if (m_vertexAccumulated == 1) {
                ConvertVertices(v, m_vertexXY);
                m_vertexAccumulated = 2;
                m_vertexCount = 1;
            }
            else {
                m_primitivesCount++;
                ConvertVertices(v, m_vertexXY + m_vertexCount);
                
                RenderTriangleClipping(m_renderTargetLimits,
                                     m_vertexXY[1024],
                                     m_vertexXY[(m_vertexCount - 1) & 1023],
                                     m_vertexXY[m_vertexCount]);
                                     
                m_vertexCount = (m_vertexCount + 1) & 1023;
                m_rasterizer->Wait();
            }
        }
        else if (m_primitiveType == PrimitiveType::TRIANGLE_STRIP) {
            if (m_vertexAccumulated == 0) {
                ConvertVertices(v, m_vertexXY);
                m_vertexAccumulated = 1;
            }
            else if (m_vertexAccumulated == 1) {
                ConvertVertices(v, m_vertexXY + 1);
                m_vertexAccumulated = 2;
                m_vertexCount = 2;
            }
            else {
                m_primitivesCount++;
                ConvertVertices(v, m_vertexXY + m_vertexCount);
                
                if ((m_vertexCount & 1) == 0) {
                    RenderTriangleClipping(m_renderTargetLimits,
                                         m_vertexXY[(m_vertexCount - 2) & 1023],
                                         m_vertexXY[(m_vertexCount - 1) & 1023],
                                         m_vertexXY[m_vertexCount]);
                }
                else {
                    RenderTriangleClipping(m_renderTargetLimits,
                                         m_vertexXY[(m_vertexCount - 2) & 1023],
                                         m_vertexXY[m_vertexCount],
                                         m_vertexXY[(m_vertexCount - 1) & 1023]);
                }
                
                m_vertexCount = (m_vertexCount + 1) & 1023;
                m_rasterizer->Wait();
            }
        }
        else if (m_primitiveType == PrimitiveType::QUAD_STRIP) {
            if (m_vertexAccumulated == 0) {
                ConvertVertices(v, m_vertexXY);
                m_vertexAccumulated = 1;
            }
            else if (m_vertexAccumulated == 1) {
                ConvertVertices(v, m_vertexXY + 1);
                m_vertexAccumulated = 2;
                m_vertexCount = 0;
            }
            else {
                ConvertVertices(v, m_vertexXY + ((m_vertexCount + m_vertexAccumulated) & 1023));
                m_vertexAccumulated++;
                
                if (m_vertexAccumulated == 4) {
                    m_primitivesCount++;
                    
                    RenderTriangleClipping(m_renderTargetLimits,
                                         m_vertexXY[m_vertexCount],
                                         m_vertexXY[(m_vertexCount + 1) & 1023],
                                         m_vertexXY[(m_vertexCount + 2) & 1023]);
                                         
                    RenderTriangleClipping(m_renderTargetLimits,
                                         m_vertexXY[(m_vertexCount + 2) & 1023],
                                         m_vertexXY[(m_vertexCount + 1) & 1023],
                                         m_vertexXY[(m_vertexCount + 3) & 1023]);
                                         
                    m_vertexAccumulated = 2;
                    m_vertexCount = (m_vertexCount + 2) & 1023;
                    m_rasterizer->Wait();
                }
            }
        }
        
        source = (source + 1) & 1023;
    }
    
    m_primitivesBatchCount += m_primitivesCount - primitivesCount;
}

void GeForce3::ConvertVertices(VertexNV* source, NV2AVertex_t* destination)
{
    if (m_vertexPipeline == 4) {
        // Transformation matrices pipeline
        float v[4] = {0};
        
        // Apply modelview matrix transformation
        for (int i = 0; i < 4; i++) {
            v[i] = 0;
            for (int j = 0; j < 4; j++) {
                v[i] += m_matrices.modelview[i][j] * source->attribute[0].fv[j];
            }
        }
        
        destination->w = v[3];
        destination->x = (v[0] / v[3]) * m_supersampleFactorX;
        destination->y = (v[1] / v[3]) * m_supersampleFactorY;
        
        float c = v[3];
        if (c == 0) c = FLT_MIN;
        c = 1.0f / c;
        
        destination->p[(int)VertexParameter::PARAM_1W] = c;
        destination->p[(int)VertexParameter::PARAM_Z] = v[2] * c;
        
        // Transform colors
        destination->p[(int)VertexParameter::PARAM_COLOR_R] = source->attribute[3].fv[0] * c;
        destination->p[(int)VertexParameter::PARAM_COLOR_G] = source->attribute[3].fv[1] * c;
        destination->p[(int)VertexParameter::PARAM_COLOR_B] = source->attribute[3].fv[2] * c;
        destination->p[(int)VertexParameter::PARAM_COLOR_A] = source->attribute[3].fv[3] * c;
        
        destination->p[(int)VertexParameter::PARAM_SECONDARY_COLOR_R] = source->attribute[4].fv[0] * c;
        destination->p[(int)VertexParameter::PARAM_SECONDARY_COLOR_G] = source->attribute[4].fv[1] * c;
        destination->p[(int)VertexParameter::PARAM_SECONDARY_COLOR_B] = source->attribute[4].fv[2] * c;
        destination->p[(int)VertexParameter::PARAM_SECONDARY_COLOR_A] = source->attribute[4].fv[3] * c;
        
        // Transform texture coordinates
        for (int u = 0; u < 4; u++) {
            destination->p[(int)VertexParameter::PARAM_TEXTURE0_S + u * 4] = source->attribute[9 + u].fv[0] * c;
            destination->p[(int)VertexParameter::PARAM_TEXTURE0_T + u * 4] = source->attribute[9 + u].fv[1] * c;
            destination->p[(int)VertexParameter::PARAM_TEXTURE0_R + u * 4] = source->attribute[9 + u].fv[2] * c;
            destination->p[(int)VertexParameter::PARAM_TEXTURE0_Q + u * 4] = source->attribute[9 + u].fv[3] * c;
        }
    }
    else {
        // Vertex program pipeline
        // Set input and output vertex data
        m_vertexInput = source;
        m_vertexOutput = &m_vertexTemp;
        
        // Execute the vertex program
        ExecuteVertexProgram();
        
        // Copy data for rendering
        destination->w = m_vertexOutput->attribute[0].fv[3];
        destination->x = (m_vertexOutput->attribute[0].fv[0] - 0.53125f) * m_supersampleFactorX;
        destination->y = (m_vertexOutput->attribute[0].fv[1] - 0.53125f) * m_supersampleFactorY;
        
        float c = destination->w;
        if (c == 0) c = FLT_MIN;
        c = 1.0f / c;
        
        destination->p[(int)VertexParameter::PARAM_1W] = c;
        destination->p[(int)VertexParameter::PARAM_Z] = m_vertexOutput->attribute[0].fv[2]; // already divided by w
        
        // Copy output colors
        destination->p[(int)VertexParameter::PARAM_COLOR_R] = m_vertexOutput->attribute[3].fv[0] * c;
        destination->p[(int)VertexParameter::PARAM_COLOR_G] = m_vertexOutput->attribute[3].fv[1] * c;
        destination->p[(int)VertexParameter::PARAM_COLOR_B] = m_vertexOutput->attribute[3].fv[2] * c;
        destination->p[(int)VertexParameter::PARAM_COLOR_A] = m_vertexOutput->attribute[3].fv[3] * c;
        
        destination->p[(int)VertexParameter::PARAM_SECONDARY_COLOR_R] = m_vertexOutput->attribute[4].fv[0] * c;
        destination->p[(int)VertexParameter::PARAM_SECONDARY_COLOR_G] = m_vertexOutput->attribute[4].fv[1] * c;
        destination->p[(int)VertexParameter::PARAM_SECONDARY_COLOR_B] = m_vertexOutput->attribute[4].fv[2] * c;
        destination->p[(int)VertexParameter::PARAM_SECONDARY_COLOR_A] = m_vertexOutput->attribute[4].fv[3] * c;
        
        // Copy output texture coordinates
        for (int u = 0; u < 4; u++) {
            destination->p[(int)VertexParameter::PARAM_TEXTURE0_S + u * 4] = m_vertexOutput->attribute[9 + u].fv[0] * c;
            destination->p[(int)VertexParameter::PARAM_TEXTURE0_T + u * 4] = m_vertexOutput->attribute[9 + u].fv[1] * c;
            destination->p[(int)VertexParameter::PARAM_TEXTURE0_R + u * 4] = m_vertexOutput->attribute[9 + u].fv[2] * c;
            destination->p[(int)VertexParameter::PARAM_TEXTURE0_Q + u * 4] = m_vertexOutput->attribute[9 + u].fv[3] * c;
        }
    }
}

// DMA and object management
void GeForce3::ReadDMAObject(uint32_t handle, uint32_t& offset, uint32_t& size)
{
    // Calculate hash to find DMA object in RAMHT
    uint32_t hash = ((((handle >> 11) ^ handle) >> 11) ^ handle) & 0x7FF;
    uint32_t ramhtOffset = (m_pfifo[0x210/4] & 0x1FF) << 8; // Base offset of RAMHT
    uint32_t entryOffset = ramhtOffset + hash * 8; // Each entry is 8 bytes
    
    // Verify the handle
    if (m_ramin[entryOffset/4] != handle) {
        // Search through RAMIN if the hash calculation fails
        for (uint32_t addr = ramhtOffset/4; addr < (sizeof(m_ramin)/4); addr += 2) {
            if (m_ramin[addr] == handle) {
                entryOffset = addr * 4;
                break;
            }
        }
    }
    
    // Extract object info from RAMIN
    uint32_t objOffset = ((m_ramin[entryOffset/4 + 1] & 0xFFFF) * 0x10);
    uint32_t objClass = m_ramin[objOffset/4] & 0xFFF;
    uint32_t dmaAdjust = (m_ramin[objOffset/4] >> 20) & 0xFFF;
    size = m_ramin[objOffset/4 + 1];
    uint32_t dmaFrame = m_ramin[objOffset/4 + 2] & 0xFFFFF000;
    
    offset = dmaFrame + dmaAdjust;
}

uint32_t GeForce3::GetTexel(int texUnit, int x, int y)
{
    uint32_t offset;
    uint32_t color;
    uint16_t a4r4g4b4, a1r5g5b5, r5g6b5;
    int blockX, blockY;
    int color0, color1, color0m2, color1m2, alpha0, alpha1;
    uint32_t codes;
    uint64_t alphas;
    int cr, cg, cb;
    int sizeU, sizeV;
    
    // Get texture dimensions
    if (!m_texture[texUnit].rectangular) {
        sizeU = m_texture[texUnit].sizeS;
        sizeV = m_texture[texUnit].sizeT;
    }
    else {
        sizeU = m_texture[texUnit].rectWidth;
        sizeV = m_texture[texUnit].rectHeight;
    }
    
    // Apply texture addressing modes
    switch (m_texture[texUnit].addrModeS) {
        default:
        case 1: // wrap
            x = x % sizeU;
            if (x < 0)
                x = sizeU + x;
            break;
            
        case 3: // clamp
            if (x < 0)
                x = 0;
            if (x >= sizeU)
                x = sizeU - 1;
            break;
    }
    
    switch (m_texture[texUnit].addrModeT) {
        default:
        case 1: // wrap
            y = y % sizeV;
            if (y < 0)
                y = sizeV + y;
            break;
            
        case 3: // clamp
            if (y < 0)
                y = 0;
            if (y >= sizeV)
                y = sizeV - 1;
            break;
    }
    
    // Sample texture based on format
    switch (m_texture[texUnit].format) {
        case TexFormat::A8R8G8B8:
            offset = Dilate0(x, m_texture[texUnit].dilate) + 
                     Dilate1(y, m_texture[texUnit].dilate);
            return *((uint32_t*)m_texture[texUnit].buffer + offset);
            
        case TexFormat::X8R8G8B8:
            offset = Dilate0(x, m_texture[texUnit].dilate) + 
                     Dilate1(y, m_texture[texUnit].dilate);
            return 0xFF000000 | (*((uint32_t*)m_texture[texUnit].buffer + offset) & 0xFFFFFF);
            
        case TexFormat::DXT1:
            blockX = x >> 2;
            blockY = y >> 2;
            x = x & 3;
            y = y & 3;
            offset = blockX + blockY * (sizeU >> 2);
            
            color0 = *((uint16_t*)(((uint64_t*)m_texture[texUnit].buffer) + offset) + 0);
            color1 = *((uint16_t*)(((uint64_t*)m_texture[texUnit].buffer) + offset) + 1);
            codes = *((uint32_t*)(((uint64_t*)m_texture[texUnit].buffer) + offset) + 1);
            
            int s = (y << 3) + (x << 1);
            int c = (codes >> s) & 3;
            c = c + (color0 > color1 ? 0 : 4);
            color0m2 = color0 << 1;
            color1m2 = color1 << 1;
            
            switch (c) {
                case 0:
                    return 0xFF000000 + ConvertR5G6B5ToRGB8(color0);
                case 1:
                    return 0xFF000000 + ConvertR5G6B5ToRGB8(color1);
                case 2:
                    cb = Pal5bit(((color0m2 & 0x003E) + (color1 & 0x001F)) / 3);
                    cg = Pal6bit(((color0m2 & 0x0FC0) + (color1 & 0x07E0)) / 3 >> 5);
                    cr = Pal5bit(((color0m2 & 0x1F000) + color1) / 3 >> 11);
                    return 0xFF000000 | (cr << 16) | (cg << 8) | cb;
                case 3:
                    cb = Pal5bit(((color1m2 & 0x003E) + (color0 & 0x001F)) / 3);
                    cg = Pal6bit(((color1m2 & 0x0FC0) + (color0 & 0x07E0)) / 3 >> 5);
                    cr = Pal5bit(((color1m2 & 0x1F000) + color0) / 3 >> 11);
                    return 0xFF000000 | (cr << 16) | (cg << 8) | cb;
                case 4:
                    return 0xFF000000 + ConvertR5G6B5ToRGB8(color0);
                case 5:
                    return 0xFF000000 + ConvertR5G6B5ToRGB8(color1);
                case 6:
                    cb = Pal5bit(((color0 & 0x001F) + (color1 & 0x001F)) / 2);
                    cg = Pal6bit(((color0 & 0x07E0) + (color1 & 0x07E0)) / 2 >> 5);
                    cr = Pal5bit(((color0 & 0xF800) + (color1 & 0xF800)) / 2 >> 11);
                    return 0xFF000000 | (cr << 16) | (cg << 8) | cb;
                default:
                    return 0xFF000000;
            }
            
        case TexFormat::A4R4G4B4:
            offset = Dilate0(x, m_texture[texUnit].dilate) + 
                     Dilate1(y, m_texture[texUnit].dilate);
            a4r4g4b4 = *(((uint16_t*)m_texture[texUnit].buffer) + offset);
            return ConvertA4R4G4B4ToARGB8(a4r4g4b4);
            
        case TexFormat::A8:
            offset = Dilate0(x, m_texture[texUnit].dilate) + 
                     Dilate1(y, m_texture[texUnit].dilate);
            color = *(((uint8_t*)m_texture[texUnit].buffer) + offset);
            return color << 24;
            
        case TexFormat::A1R5G5B5:
            offset = Dilate0(x, m_texture[texUnit].dilate) + 
                     Dilate1(y, m_texture[texUnit].dilate);
            a1r5g5b5 = *(((uint16_t*)m_texture[texUnit].buffer) + offset);
            return ConvertA1R5G5B5ToARGB8(a1r5g5b5);
            
        case TexFormat::R5G6B5:
            offset = Dilate0(x, m_texture[texUnit].dilate) + 
                     Dilate1(y, m_texture[texUnit].dilate);
            r5g6b5 = *(((uint16_t*)m_texture[texUnit].buffer) + offset);
            return 0xFF000000 + ConvertR5G6B5ToRGB8(r5g6b5);
            
        case TexFormat::A8R8G8B8_RECT:
            offset = m_texture[texUnit].rectanglePitch * y + (x << 2);
            return *((uint32_t*)(((uint8_t*)m_texture[texUnit].buffer) + offset));
            
        default:
            return 0xFF00FF00; // Magenta as "not implemented"
    }
}

uint32_t GeForce3::ConvertA4R4G4B4ToARGB8(uint32_t a4r4g4b4)
{
    uint32_t a8r8g8b8;
    int ca, cr, cg, cb;
    
    cb = Pal4bit(a4r4g4b4 & 0x000F);
    cg = Pal4bit((a4r4g4b4 & 0x00F0) >> 4);
    cr = Pal4bit((a4r4g4b4 & 0x0F00) >> 8);
    ca = Pal4bit((a4r4g4b4 & 0xF000) >> 12);
    
    a8r8g8b8 = (ca << 24) | (cr << 16) | (cg << 8) | cb;
    return a8r8g8b8;
}

uint32_t GeForce3::ConvertA1R5G5B5ToARGB8(uint32_t a1r5g5b5)
{
    uint32_t a8r8g8b8;
    int ca, cr, cg, cb;
    
    cb = Pal5bit(a1r5g5b5 & 0x001F);
    cg = Pal5bit((a1r5g5b5 & 0x03E0) >> 5);
    cr = Pal5bit((a1r5g5b5 & 0x7C00) >> 10);
    ca = a1r5g5b5 & 0x8000 ? 0xFF : 0;
    
    a8r8g8b8 = (ca << 24) | (cr << 16) | (cg << 8) | cb;
    return a8r8g8b8;
}

uint32_t GeForce3::ConvertR5G6B5ToRGB8(uint32_t r5g6b5)
{
    uint32_t r8g8b8;
    int cr, cg, cb;
    
    cb = Pal5bit(r5g6b5 & 0x001F);
    cg = Pal6bit((r5g6b5 & 0x07E0) >> 5);
    cr = Pal5bit((r5g6b5 & 0xF800) >> 11);
    
    r8g8b8 = (cr << 16) | (cg << 8) | cb;
    return r8g8b8;
}

void GeForce3::ConvertARGB8ToFloat(uint32_t color, float reg[4])
{
    reg[2] = static_cast<float>(color & 0xFF) / 255.0f;
    reg[1] = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
    reg[0] = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    reg[3] = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
}

uint32_t GeForce3::ConvertFloatToARGB8(float reg[4])
{
    uint32_t r, g, b, a;
    
    a = static_cast<uint32_t>(std::clamp(reg[3], 0.0f, 1.0f) * 255.0f);
    r = static_cast<uint32_t>(std::clamp(reg[0], 0.0f, 1.0f) * 255.0f);
    g = static_cast<uint32_t>(std::clamp(reg[1], 0.0f, 1.0f) * 255.0f);
    b = static_cast<uint32_t>(std::clamp(reg[2], 0.0f, 1.0f) * 255.0f);
    
    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t GeForce3::BilinearFilter(uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11, int xFrac, int yFrac)
{
    // Extract individual components from each pixel
    int a00 = (c00 >> 24) & 0xFF;
    int r00 = (c00 >> 16) & 0xFF;
    int g00 = (c00 >> 8) & 0xFF;
    int b00 = c00 & 0xFF;
    
    int a10 = (c10 >> 24) & 0xFF;
    int r10 = (c10 >> 16) & 0xFF;
    int g10 = (c10 >> 8) & 0xFF;
    int b10 = c10 & 0xFF;
    
    int a01 = (c01 >> 24) & 0xFF;
    int r01 = (c01 >> 16) & 0xFF;
    int g01 = (c01 >> 8) & 0xFF;
    int b01 = c01 & 0xFF;
    
    int a11 = (c11 >> 24) & 0xFF;
    int r11 = (c11 >> 16) & 0xFF;
    int g11 = (c11 >> 8) & 0xFF;
    int b11 = c11 & 0xFF;
    
    // Normalize fractions to 0-255 range
    int fx = xFrac;
    int fy = yFrac;
    
    // Calculate weights for bilinear interpolation
    int w00 = (256 - fx) * (256 - fy);
    int w10 = fx * (256 - fy);
    int w01 = (256 - fx) * fy;
    int w11 = fx * fy;
    
    // Interpolate each component
    int a = (a00 * w00 + a10 * w10 + a01 * w01 + a11 * w11) / 65536;
    int r = (r00 * w00 + r10 * w10 + r01 * w01 + r11 * w11) / 65536;
    int g = (g00 * w00 + g10 * w10 + g01 * w01 + g11 * w11) / 65536;
    int b = (b00 * w00 + b10 * w10 + b01 * w01 + b11 * w11) / 65536;
    
    // Combine components back into a single color
    return (a << 24) | (r << 16) | (g << 8) | b;
}

// Triangle rendering methods
uint32_t GeForce3::RenderTriangleClipping(const Rectangle& cliprect, NV2AVertex_t& v1, NV2AVertex_t& v2, NV2AVertex_t& v3)
{
    // Check if all vertices are in front of the near plane
    if ((v1.w > 0) && (v2.w > 0) && (v3.w > 0))
        return RenderTriangleCulling(cliprect, v1, v2, v3);
    
    // If W-clipping is disabled, return 0
    if (!m_enableClippingW)
        return 0;
    
    // If all vertices are behind the near plane, discard the triangle
    if ((v1.w <= 0) && (v2.w <= 0) && (v3.w <= 0))
        return 0;
    
    // We need to clip the triangle
    NV2AVertex_t* vp[3];
    NV2AVertex_t vi[3];
    NV2AVertex_t vo[8];
    int numVerts;
    double clip;
    
    // Assign the elements of the pointer array
    vp[0] = &v1;
    vp[1] = &v2;
    vp[2] = &v3;
    
    // Convert vertices back to pre-perspective divide state
    if (m_vertexPipeline == 4) {
        for (int n = 0; n < 3; n++) {
            clip = vp[n]->w;
            vi[n].w = clip;
            vi[n].x = (vp[n]->x / m_supersampleFactorX) * clip;
            vi[n].y = (vp[n]->y / m_supersampleFactorY) * clip;
            
            for (int param = 0; param <= (int)VertexParameter::ALL; param++) {
                vi[n].p[param] = vp[n]->p[param] * clip;
            }
        }
    }
    else {
        for (int n = 0; n < 3; n++) {
            clip = vp[n]->w;
            vi[n].w = clip;
            
            // Remove perspective correct interpolate
            for (int param = 0; param < (int)VertexParameter::PARAM_Z; param++) {
                vi[n].p[param] = vp[n]->p[param] * clip;
            }
            
            // Remove supersample
            vi[n].x = (vp[n]->x / m_supersampleFactorX) + 0.53125f;
            vi[n].y = (vp[n]->y / m_supersampleFactorY) + 0.53125f;
            
            // Remove translate
            vi[n].x = vi[n].x - m_matrices.translate[0];
            vi[n].y = vi[n].y - m_matrices.translate[1];
            vi[n].p[(int)VertexParameter::PARAM_Z] = vp[n]->p[(int)VertexParameter::PARAM_Z] - m_matrices.translate[2];
            
            // Remove perspective divide
            vi[n].x = vi[n].x * clip;
            vi[n].y = vi[n].y * clip;
            vi[n].p[(int)VertexParameter::PARAM_Z] = vi[n].p[(int)VertexParameter::PARAM_Z] * clip;
        }
    }
    
    // Clip the triangle against the w=0 plane
    numVerts = ClipTriangleW(vi, vo);
    
    // Convert the clipped vertices back to screen coordinates
    if (m_vertexPipeline == 4) {
        for (int n = 0; n < numVerts; n++) {
            clip = 1.0 / vo[n].w;
            vo[n].x = vo[n].x * m_supersampleFactorX * clip;
            vo[n].y = vo[n].y * m_supersampleFactorY * clip;
            
            for (int param = 0; param <= (int)VertexParameter::ALL; param++) {
                vo[n].p[param] = vo[n].p[param] * clip;
            }
        }
    }
    else {
        for (int n = 0; n < numVerts; n++) {
            clip = 1.0 / vo[n].w;
            
            // Apply perspective divide
            vo[n].x = vo[n].x * clip;
            vo[n].y = vo[n].y * clip;
            vo[n].p[(int)VertexParameter::PARAM_Z] = vo[n].p[(int)VertexParameter::PARAM_Z] * clip;
            
            // Apply translate
            vo[n].x = vo[n].x + m_matrices.translate[0];
            vo[n].y = vo[n].y + m_matrices.translate[1];
            vo[n].p[(int)VertexParameter::PARAM_Z] = vo[n].p[(int)VertexParameter::PARAM_Z] + m_matrices.translate[2];
            
            // Apply supersample
            vo[n].x = (vo[n].x - 0.53125f) * m_supersampleFactorX;
            vo[n].y = (vo[n].y - 0.53125f) * m_supersampleFactorY;
            
            // Apply perspective correct interpolate
            for (int param = 0; param < (int)VertexParameter::PARAM_Z; param++) {
                vo[n].p[param] = vo[n].p[param] * clip;
            }
        }
    }
    
    // Render the resulting polygon as a fan of triangles
    for (int n = 1; n <= (numVerts - 2); n++) {
        RenderTriangleCulling(cliprect, vo[0], vo[n], vo[n + 1]);
    }
    
    return 0;
}

int GeForce3::ClipTriangleW(NV2AVertex_t vi[3], NV2AVertex_t* vo)
{
    int idxPrev, idxCurr;
    int negPrev, negCurr;
    double tfactor;
    int idx;
    const double wthreshold = 0.000001;
    
    idxPrev = 2;
    idxCurr = 0;
    idx = 0;
    negPrev = vi[idxPrev].w < wthreshold ? 1 : 0;
    
    while (idxCurr < 3) {
        negCurr = vi[idxCurr].w < wthreshold ? 1 : 0;
        
        if (negCurr ^ negPrev) {
            // Edge intersects the W=0 plane, calculate intersection point
            tfactor = (wthreshold - vi[idxPrev].w) / (vi[idxCurr].w - vi[idxPrev].w);
            
            // Linear interpolate all vertex attributes
            vo[idx].x = ((vi[idxCurr].x - vi[idxPrev].x) * tfactor) + vi[idxPrev].x;
            vo[idx].y = ((vi[idxCurr].y - vi[idxPrev].y) * tfactor) + vi[idxPrev].y;
            vo[idx].w = ((vi[idxCurr].w - vi[idxPrev].w) * tfactor) + vi[idxPrev].w;
            
            for (int n = 0; n < (int)VertexParameter::ALL; n++) {
                vo[idx].p[n] = ((vi[idxCurr].p[n] - vi[idxPrev].p[n]) * tfactor) + vi[idxPrev].p[n];
            }
            
            vo[idx].p[(int)VertexParameter::PARAM_1W] = 1.0f / vo[idx].w;
            idx++;
        }
        
        // Add current vertex if it's on the positive side
        if (negCurr == 0) {
            vo[idx].x = vi[idxCurr].x;
            vo[idx].y = vi[idxCurr].y;
            vo[idx].w = vi[idxCurr].w;
            
            for (int n = 0; n < (int)VertexParameter::ALL; n++) {
                vo[idx].p[n] = vi[idxCurr].p[n];
            }
            
            vo[idx].p[(int)VertexParameter::PARAM_1W] = 1.0f / vo[idx].w;
            idx++;
        }
        
        negPrev = negCurr;
        idxPrev = idxCurr;
        idxCurr++;
    }
    
    return idx;
}

uint32_t GeForce3::RenderTriangleCulling(const Rectangle& cliprect, NV2AVertex_t& v1, NV2AVertex_t& v2, NV2AVertex_t& v3)
{
    float areax2;
    CullingFace face = CullingFace::FRONT;
    
    // If backface culling is disabled, just render the triangle
    if (!m_backfaceCullingEnabled)
        return m_rasterizer->RenderTriangle<(int)VertexParameter::ALL>(cliprect, 
                                                                     m_renderCallback, 
                                                                     static_cast<void*>(this),
                                                                     v1, v2, v3);
    
    // If culling front and back, discard the triangle
    if (m_backfaceCullingFace == CullingFace::FRONT_AND_BACK) {
        m_trianglesBfCulled++;
        return 0;
    }
    
    // Calculate the signed area of the triangle
    areax2 = v1.x * (v2.y - v3.y) + v2.x * (v3.y - v1.y) + v3.x * (v1.y - v2.y);
    
    // Degenerate triangle case
    if (areax2 == 0.0f) {
        m_trianglesBfCulled++;
        return 0;
    }
    
    // Determine the face based on the winding order
    if (m_backfaceCullingWinding == CullingWinding::CCW) {
        face = (-areax2 <= 0) ? CullingFace::BACK : CullingFace::FRONT;
    } else {
        face = (areax2 <= 0) ? CullingFace::BACK : CullingFace::FRONT;
    }
    
    // Check if we should cull this face
    if ((face == CullingFace::FRONT && m_backfaceCullingFace == CullingFace::BACK) ||
        (face == CullingFace::BACK && m_backfaceCullingFace == CullingFace::FRONT)) {
        return m_rasterizer->RenderTriangle<(int)VertexParameter::ALL>(cliprect, 
                                                                     m_renderCallback, 
                                                                     static_cast<void*>(this), 
                                                                     v1, v2, v3);
    }
    
    // Triangle is being culled
    m_trianglesBfCulled++;
    return 0;
}

void GeForce3::RenderColor(int32_t scanline, const RasterizerExtent_t& extent, void* objectData, int threadId)
{
    int x, lastX;
    
    lastX = m_renderTargetLimits.right();
    int startX = extent.startx;
    int stopX = extent.stopx;
    
    if ((startX < 0) && (stopX <= 0))
        return;
    if ((startX > lastX) && (stopX > lastX))
        return;
    
    int numPixels = stopX - startX; // Number of pixels to draw
    
    if (stopX > lastX)
        numPixels = numPixels - (stopX - lastX - 1);
    
    numPixels--;
    x = numPixels;
    
    while (x >= 0) {
        double zBuffer;
        uint32_t a8r8g8b8;
        int z;
        int ca, cr, cg, cb;
        int xPos = startX + x; // X coordinate of current pixel
        
        // Calculate depth value
        z = static_cast<int>(extent.param[(int)VertexParameter::PARAM_Z].start + 
                            (double)x * extent.param[(int)VertexParameter::PARAM_Z].dpdx);
        
        // Calculate 1/W value for perspective-correct interpolation
        double wFactor = extent.param[(int)VertexParameter::PARAM_1W].start + 
                        (double)x * extent.param[(int)VertexParameter::PARAM_1W].dpdx;
        wFactor = 1.0f / wFactor;
        
        // Calculate color components with perspective correction
        cb = std::clamp<int>(
            (extent.param[(int)VertexParameter::PARAM_COLOR_B].start + 
             (double)x * extent.param[(int)VertexParameter::PARAM_COLOR_B].dpdx) * wFactor * 255.0f,
            0, 255);
            
        cg = std::clamp<int>(
            (extent.param[(int)VertexParameter::PARAM_COLOR_G].start + 
             (double)x * extent.param[(int)VertexParameter::PARAM_COLOR_G].dpdx) * wFactor * 255.0f,
            0, 255);
            
        cr = std::clamp<int>(
            (extent.param[(int)VertexParameter::PARAM_COLOR_R].start + 
             (double)x * extent.param[(int)VertexParameter::PARAM_COLOR_R].dpdx) * wFactor * 255.0f,
            0, 255);
            
        ca = std::clamp<int>(
            (extent.param[(int)VertexParameter::PARAM_COLOR_A].start + 
             (double)x * extent.param[(int)VertexParameter::PARAM_COLOR_A].dpdx) * wFactor * 255.0f,
            0, 255);
        
        // Create final pixel color
        a8r8g8b8 = (ca << 24) | (cr << 16) | (cg << 8) | cb;
        
        // Write the pixel
        WritePixel(xPos, scanline, a8r8g8b8, z);
        
        x--;
    }
}

void GeForce3::RenderTextureSimple(int32_t scanline, const RasterizerExtent_t& extent, void* objectData, int threadId)
{
    int sizeX, limitX;
    double mx, my;
    uint32_t a8r8g8b8;
    
    // If texture is disabled, return
    if (!m_texture[0].enabled) {
        return;
    }
    
    // Calculate texture coordinates scaling
    limitX = m_renderTargetLimits.right();
    sizeX = extent.stopx - extent.startx; // Number of pixels to draw
    
    if (extent.stopx > limitX)
        sizeX = sizeX - (extent.stopx - limitX - 1);
    
    if (m_texture[0].rectangular) {
        mx = my = 1 * 256;
    } else {
        mx = m_texture[0].sizeS * 256;
        my = m_texture[0].sizeT * 256;
    }
    
    double zDepth = extent.param[(int)VertexParameter::PARAM_Z].start;
    double wFactor = extent.param[(int)VertexParameter::PARAM_1W].start;
    double sCoord = extent.param[(int)VertexParameter::PARAM_TEXTURE0_S].start;
    double tCoord = extent.param[(int)VertexParameter::PARAM_TEXTURE0_T].start;
    
    for (int pos = 0; pos < sizeX; pos++) {
        int xPos = extent.startx + pos; // X coordinate of current pixel
        double w = 1.0f / wFactor;
        
        // Calculate texture coordinates with perspective correction
        double s = sCoord * w;
        double t = tCoord * w;
        
        // Scale texture coordinates to texture space
        s = s * mx;
        t = t * my;
        
        // Get Z value
        int z = zDepth;
        
        // Calculate texel coordinates
        int pixelX = static_cast<int>(s);
        int pixelY = static_cast<int>(t);
        int pixelXFrac = pixelX & 255;
        int pixelYFrac = pixelY & 255;
        pixelX >>= 8;
        pixelY >>= 8;
        
        // Sample texture with or without bilinear filtering
        if (m_bilinearFilter) {
            uint32_t c00 = GetTexel(0, pixelX + 0, pixelY + 0);
            uint32_t c01 = GetTexel(0, pixelX + 0, pixelY + 1);
            uint32_t c10 = GetTexel(0, pixelX + 1, pixelY + 0);
            uint32_t c11 = GetTexel(0, pixelX + 1, pixelY + 1);
            
            // Perform bilinear filtering
            a8r8g8b8 = BilinearFilter(c00, c10, c01, c11, pixelXFrac, pixelYFrac);
        } else {
            a8r8g8b8 = GetTexel(0, pixelX, pixelY);
        }
        
        // Write the pixel
        WritePixel(xPos, scanline, a8r8g8b8, z);
        
        // Step for the next pixel
        zDepth += extent.param[(int)VertexParameter::PARAM_Z].dpdx;
        wFactor += extent.param[(int)VertexParameter::PARAM_1W].dpdx;
        sCoord += extent.param[(int)VertexParameter::PARAM_TEXTURE0_S].dpdx;
        tCoord += extent.param[(int)VertexParameter::PARAM_TEXTURE0_T].dpdx;
    }
}

void GeForce3::RenderRegisterCombiners(int32_t scanline, const RasterizerExtent_t& extent, void* objectData, int threadId)
{
    int sizeX, limitX;
    float colorF[7][4];
    double mx[4], my[4];
    double sCoord[4], tCoord[4], rCoord[4], qCoord[4];
    uint32_t colors[6];
    uint32_t finalColor;
    
    // Initialize colors array
    for (int i = 0; i < 6; i++)
        colors[i] = 0;
    
    // Calculate rendering limits
    limitX = m_renderTargetLimits.right();
    sizeX = extent.stopx - extent.startx;
    
    if ((extent.startx < 0) && (extent.stopx <= 0))
        return;
    if ((extent.startx > limitX) && (extent.stopx > limitX))
        return;
    if (extent.stopx > limitX)
        sizeX = sizeX - (extent.stopx - limitX - 1);
    
    // Initialize texture coordinate scaling
    for (int n = 0; n < 4; n++) {
        if (m_texture[n].rectangular) {
            mx[n] = my[n] = 1 * 256;
        } else {
            mx[n] = m_texture[n].sizeS * 256;
            my[n] = m_texture[n].sizeT * 256;
        }
        
        sCoord[n] = extent.param[(int)VertexParameter::PARAM_TEXTURE0_S + n * 4].start;
        tCoord[n] = extent.param[(int)VertexParameter::PARAM_TEXTURE0_T + n * 4].start;
        rCoord[n] = extent.param[(int)VertexParameter::PARAM_TEXTURE0_R + n * 4].start;
        qCoord[n] = extent.param[(int)VertexParameter::PARAM_TEXTURE0_Q + n * 4].start;
    }
    
    // Initialize vertex color coordinates
    double c00 = extent.param[(int)VertexParameter::PARAM_COLOR_R].start;
    double c01 = extent.param[(int)VertexParameter::PARAM_COLOR_G].start;
    double c02 = extent.param[(int)VertexParameter::PARAM_COLOR_B].start;
    double c03 = extent.param[(int)VertexParameter::PARAM_COLOR_A].start;
    
    double c10 = extent.param[(int)VertexParameter::PARAM_SECONDARY_COLOR_R].start;
    double c11 = extent.param[(int)VertexParameter::PARAM_SECONDARY_COLOR_G].start;
    double c12 = extent.param[(int)VertexParameter::PARAM_SECONDARY_COLOR_B].start;
    double c13 = extent.param[(int)VertexParameter::PARAM_SECONDARY_COLOR_A].start;
    
    double zDepth = extent.param[(int)VertexParameter::PARAM_Z].start;
    double wFactor = extent.param[(int)VertexParameter::PARAM_1W].start;
    
    // Process each pixel in the span
    for (int pos = 0; pos < sizeX; pos++) {
        int xPos = extent.startx + pos;
        int z = zDepth;
        double w = 1.0f / wFactor;
        
        // 1: Fetch data
        // 1.1: Interpolated color from vertices
        colorF[0][0] = c00 * w;
        colorF[0][1] = c01 * w;
        colorF[0][2] = c02 * w;
        colorF[0][3] = c03 * w;
        
        colorF[1][0] = c10 * w;
        colorF[1][1] = c11 * w;
        colorF[1][2] = c12 * w;
        colorF[1][3] = c13 * w;
        
        // 1.2: Coordinates for each of the 4 possible textures
        for (int n = 0; n < 4; n++) {
            colorF[n + 2][0] = sCoord[n] * w;
            colorF[n + 2][1] = tCoord[n] * w;
            colorF[n + 2][2] = rCoord[n] * w;
            colorF[n + 2][3] = qCoord[n] * w;
        }
        
        // 1.3: Fog
        ConvertARGB8ToFloat(m_fogColor, colorF[6]);
        colorF[6][3] = 1.0f; // Should it be from the vertex shader?
        
        // 1.4: Colors from textures
        for (int n = 0; n < 4; n++) {
            if (m_texture[n].mode == 1) {
                // Calculate texture coordinates
                double s = colorF[n + 2][0] * mx[n];
                double t = colorF[n + 2][1] * my[n];
                int px = static_cast<int>(s);
                int py = static_cast<int>(t);
                int pxFrac = px & 255;
                int pyFrac = py & 255;
                px >>= 8;
                py >>= 8;
                
                // Sample the texture
                if (m_bilinearFilter) {
                    uint32_t c00 = GetTexel(n, px + 0, py + 0);
                    uint32_t c01 = GetTexel(n, px + 0, py + 1);
                    uint32_t c10 = GetTexel(n, px + 1, py + 0);
                    uint32_t c11 = GetTexel(n, px + 1, py + 1);
                    
                    // Apply bilinear filtering
                    finalColor = BilinearFilter(c00, c10, c01, c11, pxFrac, pyFrac);
                } else {
                    finalColor = GetTexel(n, px, py);
                }
                
                ConvertARGB8ToFloat(finalColor, colorF[n + 2]);
            } else if (m_texture[n].mode == 4) {
                // Special handling for texture mode 4
                // Do nothing for now
            } else {
                // Default to black with alpha
                ConvertARGB8ToFloat(0xFF000000, colorF[n + 2]);
            }
        }
        
        // 2: Compute
        // 2.1: Initialize combiner registers
        InitializeCombinerRegisters(threadId, colorF);
        
        // 2.2: Process general combiner stages
        for (int n = 0; n < m_combiner.setup.stages; n++) {
            // Initialize stage
            InitializeCombinerStage(threadId, n);
            
            // Map inputs
            MapCombinerStageInput(threadId, n);
            
            // Compute outputs
            ComputeRGBOutputs(threadId, n);
            ComputeAlphaOutputs(threadId, n);
            
            // Map outputs to registers
            MapCombinerStageOutput(threadId, n);
        }
        
        // 2.3: Process final combiner stage
        InitializeFinalCombiner(threadId);
        MapFinalCombinerInput(threadId);
        FinalCombinerOutput(threadId);
        
        // Get the final color
        finalColor = ConvertFloatToARGB8(m_combiner.work[threadId].output);
        
        // 3: Write pixel
        WritePixel(xPos, scanline, finalColor, z);
        
        // Step for the next pixel
        zDepth += extent.param[(int)VertexParameter::PARAM_Z].dpdx;
        wFactor += extent.param[(int)VertexParameter::PARAM_1W].dpdx;
        
        for (int n = 0; n < 4; n++) {
            sCoord[n] += extent.param[(int)VertexParameter::PARAM_TEXTURE0_S + n * 4].dpdx;
            tCoord[n] += extent.param[(int)VertexParameter::PARAM_TEXTURE0_T + n * 4].dpdx;
            rCoord[n] += extent.param[(int)VertexParameter::PARAM_TEXTURE0_R + n * 4].dpdx;
            qCoord[n] += extent.param[(int)VertexParameter::PARAM_TEXTURE0_Q + n * 4].dpdx;
        }
        
        c00 += extent.param[(int)VertexParameter::PARAM_COLOR_R].dpdx;
        c01 += extent.param[(int)VertexParameter::PARAM_COLOR_G].dpdx;
        c02 += extent.param[(int)VertexParameter::PARAM_COLOR_B].dpdx;
        c03 += extent.param[(int)VertexParameter::PARAM_COLOR_A].dpdx;
        
        c10 += extent.param[(int)VertexParameter::PARAM_SECONDARY_COLOR_R].dpdx;
        c11 += extent.param[(int)VertexParameter::PARAM_SECONDARY_COLOR_G].dpdx;
        c12 += extent.param[(int)VertexParameter::PARAM_SECONDARY_COLOR_B].dpdx;
        c13 += extent.param[(int)VertexParameter::PARAM_SECONDARY_COLOR_A].dpdx;
    }
}

// Combiner methods
float GeForce3::CombinerMapInputFunction(CombinerMapFunction code, float value)
{
    switch (code) {
        case CombinerMapFunction::UNSIGNED_IDENTITY:
            return std::max(0.0f, value);
            
        case CombinerMapFunction::UNSIGNED_INVERT:
            return 1.0f - std::clamp(value, 0.0f, 1.0f);
            
        case CombinerMapFunction::EXPAND_NORMAL:
            return 2.0f * std::max(0.0f, value) - 1.0f;
            
        case CombinerMapFunction::EXPAND_NEGATE:
            return -2.0f * std::max(0.0f, value) + 1.0f;
            
        case CombinerMapFunction::HALF_BIAS_NORMAL:
            return std::max(0.0f, value) - 0.5f;
            
        case CombinerMapFunction::HALF_BIAS_NEGATE:
            return -std::max(0.0f, value) + 0.5f;
            
        case CombinerMapFunction::SIGNED_IDENTITY:
            return value;
            
        case CombinerMapFunction::SIGNED_NEGATE:
        default:
            return -value;
    }
}

void GeForce3::CombinerMapInputFunctionArray(CombinerMapFunction code, float* data)
{
    switch (code) {
        case CombinerMapFunction::UNSIGNED_IDENTITY:
            data[0] = std::max(0.0f, data[0]);
            data[1] = std::max(0.0f, data[1]);
            data[2] = std::max(0.0f, data[2]);
            break;
            
        case CombinerMapFunction::UNSIGNED_INVERT:
            data[0] = 1.0f - std::clamp(data[0], 0.0f, 1.0f);
            data[1] = 1.0f - std::clamp(data[1], 0.0f, 1.0f);
            data[2] = 1.0f - std::clamp(data[2], 0.0f, 1.0f);
            break;
            
        case CombinerMapFunction::EXPAND_NORMAL:
            data[0] = 2.0f * std::max(0.0f, data[0]) - 1.0f;
            data[1] = 2.0f * std::max(0.0f, data[1]) - 1.0f;
            data[2] = 2.0f * std::max(0.0f, data[2]) - 1.0f;
            break;
            
        case CombinerMapFunction::EXPAND_NEGATE:
            data[0] = -2.0f * std::max(0.0f, data[0]) + 1.0f;
            data[1] = -2.0f * std::max(0.0f, data[1]) + 1.0f;
            data[2] = -2.0f * std::max(0.0f, data[2]) + 1.0f;
            break;
            
        case CombinerMapFunction::HALF_BIAS_NORMAL:
            data[0] = std::max(0.0f, data[0]) - 0.5f;
            data[1] = std::max(0.0f, data[1]) - 0.5f;
            data[2] = std::max(0.0f, data[2]) - 0.5f;
            break;
            
        case CombinerMapFunction::HALF_BIAS_NEGATE:
            data[0] = -std::max(0.0f, data[0]) + 0.5f;
            data[1] = -std::max(0.0f, data[1]) + 0.5f;
            data[2] = -std::max(0.0f, data[2]) + 0.5f;
            break;
            
        case CombinerMapFunction::SIGNED_IDENTITY:
            return;
            
        case CombinerMapFunction::SIGNED_NEGATE:
        default:
            data[0] = -data[0];
            data[1] = -data[1];
            data[2] = -data[2];
            break;
    }
}

void GeForce3::InitializeCombinerRegisters(int threadId, float rgba[7][4])
{
    for (int n = 0; n < 4; n++) {
        // Initialize main color registers
        m_combiner.work[threadId].registers.primaryColor[n] = rgba[0][n];
        m_combiner.work[threadId].registers.secondaryColor[n] = rgba[1][n];
        m_combiner.work[threadId].registers.texture0Color[n] = rgba[2][n];
        m_combiner.work[threadId].registers.texture1Color[n] = rgba[3][n];
        m_combiner.work[threadId].registers.texture2Color[n] = rgba[4][n];
        m_combiner.work[threadId].registers.texture3Color[n] = rgba[5][n];
        m_combiner.work[threadId].registers.fogColor[n] = rgba[6][n];
    }
    
    // Alpha of spare0 must be the alpha of the pixel from texture0
    m_combiner.work[threadId].registers.spare0[3] = m_combiner.work[threadId].registers.texture0Color[3];
    
    // Zero register
    m_combiner.work[threadId].registers.zero[0] = 0.0f;
    m_combiner.work[threadId].registers.zero[1] = 0.0f;
    m_combiner.work[threadId].registers.zero[2] = 0.0f;
    m_combiner.work[threadId].registers.zero[3] = 0.0f;
}

void GeForce3::InitializeCombinerStage(int threadId, int stageNumber)
{
    int n = stageNumber;
    
    // Copy constant colors to color registers
    for (int i = 0; i < 4; i++) {
        m_combiner.work[threadId].registers.color0[i] = m_combiner.setup.stage[n].constantColor0[i];
        m_combiner.work[threadId].registers.color1[i] = m_combiner.setup.stage[n].constantColor1[i];
    }
}

void GeForce3::InitializeFinalCombiner(int threadId)
{
    // Copy constant colors to color registers
    for (int i = 0; i < 4; i++) {
        m_combiner.work[threadId].registers.color0[i] = m_combiner.setup.final.constantColor0[i];
        m_combiner.work[threadId].registers.color1[i] = m_combiner.setup.final.constantColor1[i];
    }
}

float GeForce3::CombinerMapInputSelect(int threadId, CombinerInputRegister code, int index)
{
    float* reg = nullptr;
    
    switch (code) {
        case CombinerInputRegister::ZERO:
        default:
            reg = m_combiner.work[threadId].registers.zero;
            break;
        case CombinerInputRegister::COLOR0:
            reg = m_combiner.work[threadId].registers.color0;
            break;
        case CombinerInputRegister::COLOR1:
            reg = m_combiner.work[threadId].registers.color1;
            break;
        case CombinerInputRegister::FOG_COLOR:
            reg = m_combiner.work[threadId].registers.fogColor;
            break;
        case CombinerInputRegister::PRIMARY_COLOR:
            reg = m_combiner.work[threadId].registers.primaryColor;
            break;
        case CombinerInputRegister::SECONDARY_COLOR:
            reg = m_combiner.work[threadId].registers.secondaryColor;
            break;
        case CombinerInputRegister::TEXTURE0_COLOR:
            reg = m_combiner.work[threadId].registers.texture0Color;
            break;
        case CombinerInputRegister::TEXTURE1_COLOR:
            reg = m_combiner.work[threadId].registers.texture1Color;
            break;
        case CombinerInputRegister::TEXTURE2_COLOR:
            reg = m_combiner.work[threadId].registers.texture2Color;
            break;
        case CombinerInputRegister::TEXTURE3_COLOR:
            reg = m_combiner.work[threadId].registers.texture3Color;
            break;
        case CombinerInputRegister::SPARE0:
            reg = m_combiner.work[threadId].registers.spare0;
            break;
        case CombinerInputRegister::SPARE1:
            reg = m_combiner.work[threadId].registers.spare1;
            break;
        case CombinerInputRegister::SUM_CLAMP:
            reg = m_combiner.work[threadId].variables.sumClamp;
            break;
        case CombinerInputRegister::EF:
            reg = m_combiner.work[threadId].variables.EF;
            break;
    }
    
    return reg ? reg[index] : 0.0f;
}

float* GeForce3::CombinerMapInputSelectArray(int threadId, CombinerInputRegister code)
{
    switch (code) {
        case CombinerInputRegister::ZERO:
        default:
            return m_combiner.work[threadId].registers.zero;
        case CombinerInputRegister::COLOR0:
            return m_combiner.work[threadId].registers.color0;
        case CombinerInputRegister::COLOR1:
            return m_combiner.work[threadId].registers.color1;
        case CombinerInputRegister::FOG_COLOR:
            return m_combiner.work[threadId].registers.fogColor;
        case CombinerInputRegister::PRIMARY_COLOR:
            return m_combiner.work[threadId].registers.primaryColor;
        case CombinerInputRegister::SECONDARY_COLOR:
            return m_combiner.work[threadId].registers.secondaryColor;
        case CombinerInputRegister::TEXTURE0_COLOR:
            return m_combiner.work[threadId].registers.texture0Color;
        case CombinerInputRegister::TEXTURE1_COLOR:
            return m_combiner.work[threadId].registers.texture1Color;
        case CombinerInputRegister::TEXTURE2_COLOR:
            return m_combiner.work[threadId].registers.texture2Color;
        case CombinerInputRegister::TEXTURE3_COLOR:
            return m_combiner.work[threadId].registers.texture3Color;
        case CombinerInputRegister::SPARE0:
            return m_combiner.work[threadId].registers.spare0;
        case CombinerInputRegister::SPARE1:
            return m_combiner.work[threadId].registers.spare1;
        case CombinerInputRegister::SUM_CLAMP:
            return m_combiner.work[threadId].variables.sumClamp;
        case CombinerInputRegister::EF:
            return m_combiner.work[threadId].variables.EF;
    }
}

float* GeForce3::CombinerMapOutputSelectArray(int threadId, CombinerInputRegister code)
{
    switch (code) {
        case CombinerInputRegister::PRIMARY_COLOR:
            return m_combiner.work[threadId].registers.primaryColor;
        case CombinerInputRegister::SECONDARY_COLOR:
            return m_combiner.work[threadId].registers.secondaryColor;
        case CombinerInputRegister::TEXTURE0_COLOR:
            return m_combiner.work[threadId].registers.texture0Color;
        case CombinerInputRegister::TEXTURE1_COLOR:
            return m_combiner.work[threadId].registers.texture1Color;
        case CombinerInputRegister::TEXTURE2_COLOR:
            return m_combiner.work[threadId].registers.texture2Color;
        case CombinerInputRegister::TEXTURE3_COLOR:
            return m_combiner.work[threadId].registers.texture3Color;
        case CombinerInputRegister::SPARE0:
            return m_combiner.work[threadId].registers.spare0;
        case CombinerInputRegister::SPARE1:
            return m_combiner.work[threadId].registers.spare1;
        case CombinerInputRegister::ZERO:
        case CombinerInputRegister::COLOR0:
        case CombinerInputRegister::COLOR1:
        case CombinerInputRegister::FOG_COLOR:
        case CombinerInputRegister::SUM_CLAMP:
        case CombinerInputRegister::EF:
        default:
            return nullptr;
    }
}

void GeForce3::MapCombinerStageInput(int threadId, int stageNumber)
{
    int n = stageNumber;
    int c, d, i;
    float* pv;
    
    // RGB portion
    // A
    pv = CombinerMapInputSelectArray(threadId, m_combiner.setup.stage[n].mapinRGB.aInput);
    c = m_combiner.setup.stage[n].mapinRGB.aComponent * 3;
    i = m_combiner.setup.stage[n].mapinRGB.aComponent ^ 1;
    
    // Copy components to A
    for (d = 0; d < 3; d++) {
        m_combiner.work[threadId].variables.A[d] = pv[c];
        c += i;
    }
    
    // Apply mapping function
    CombinerMapInputFunctionArray(m_combiner.setup.stage[n].mapinRGB.aMapping, 
                               m_combiner.work[threadId].variables.A);
                               
    // B
    pv = CombinerMapInputSelectArray(threadId, m_combiner.setup.stage[n].mapinRGB.bInput);
    c = m_combiner.setup.stage[n].mapinRGB.bComponent * 3;
    i = m_combiner.setup.stage[n].mapinRGB.bComponent ^ 1;
    
    for (d = 0; d < 3; d++) {
        m_combiner.work[threadId].variables.B[d] = pv[c];
        c += i;
    }
    
    CombinerMapInputFunctionArray(m_combiner.setup.stage[n].mapinRGB.bMapping, 
                               m_combiner.work[threadId].variables.B);
                               
    // C
    pv = CombinerMapInputSelectArray(threadId, m_combiner.setup.stage[n].mapinRGB.cInput);
    c = m_combiner.setup.stage[n].mapinRGB.cComponent * 3;
    i = m_combiner.setup.stage[n].mapinRGB.cComponent ^ 1;
    
    for (d = 0; d < 3; d++) {
        m_combiner.work[threadId].variables.C[d] = pv[c];
        c += i;
    }
    
    CombinerMapInputFunctionArray(m_combiner.setup.stage[n].mapinRGB.cMapping, 
                               m_combiner.work[threadId].variables.C);
                               
    // D
    pv = CombinerMapInputSelectArray(threadId, m_combiner.setup.stage[n].mapinRGB.dInput);
    c = m_combiner.setup.stage[n].mapinRGB.dComponent * 3;
    i = m_combiner.setup.stage[n].mapinRGB.dComponent ^ 1;
    
    for (d = 0; d < 3; d++) {
        m_combiner.work[threadId].variables.D[d] = pv[c];
        c += i;
    }
    
    CombinerMapInputFunctionArray(m_combiner.setup.stage[n].mapinRGB.dMapping, 
                               m_combiner.work[threadId].variables.D);
    
    // Alpha portion
    // A
    float v = CombinerMapInputSelect(threadId, 
                                  m_combiner.setup.stage[n].mapinAlpha.aInput, 
                                  2 + m_combiner.setup.stage[n].mapinAlpha.aComponent);
                                  
    m_combiner.work[threadId].variables.A[3] = CombinerMapInputFunction(
        m_combiner.setup.stage[n].mapinAlpha.aMapping, v);
        
    // B
    v = CombinerMapInputSelect(threadId, 
                            m_combiner.setup.stage[n].mapinAlpha.bInput, 
                            2 + m_combiner.setup.stage[n].mapinAlpha.bComponent);
                            
    m_combiner.work[threadId].variables.B[3] = CombinerMapInputFunction(
        m_combiner.setup.stage[n].mapinAlpha.bMapping, v);
        
    // C
    v = CombinerMapInputSelect(threadId, 
                            m_combiner.setup.stage[n].mapinAlpha.cInput, 
                            2 + m_combiner.setup.stage[n].mapinAlpha.cComponent);
                            
    m_combiner.work[threadId].variables.C[3] = CombinerMapInputFunction(
        m_combiner.setup.stage[n].mapinAlpha.cMapping, v);
        
    // D
    v = CombinerMapInputSelect(threadId, 
                            m_combiner.setup.stage[n].mapinAlpha.dInput, 
                            2 + m_combiner.setup.stage[n].mapinAlpha.dComponent);
                            
    m_combiner.work[threadId].variables.D[3] = CombinerMapInputFunction(
        m_combiner.setup.stage[n].mapinAlpha.dMapping, v);
}

void GeForce3::MapCombinerStageOutput(int threadId, int stageNumber)
{
    int n = stageNumber;
    float* f;
    
    // RGB
    f = CombinerMapOutputSelectArray(threadId, m_combiner.setup.stage[n].mapoutRGB.abOutput);
    if (f) {
        f[0] = m_combiner.work[threadId].functions.RGBop1[0];
        f[1] = m_combiner.work[threadId].functions.RGBop1[1];
        f[2] = m_combiner.work[threadId].functions.RGBop1[2];
    }
    
    f = CombinerMapOutputSelectArray(threadId, m_combiner.setup.stage[n].mapoutRGB.cdOutput);
    if (f) {
        f[0] = m_combiner.work[threadId].functions.RGBop2[0];
        f[1] = m_combiner.work[threadId].functions.RGBop2[1];
        f[2] = m_combiner.work[threadId].functions.RGBop2[2];
    }
    
    if ((m_combiner.setup.stage[n].mapoutRGB.abDotProduct | m_combiner.setup.stage[n].mapoutRGB.cdDotProduct) == 0) {
        f = CombinerMapOutputSelectArray(threadId, m_combiner.setup.stage[n].mapoutRGB.sumOutput);
        if (f) {
            f[0] = m_combiner.work[threadId].functions.RGBop3[0];
            f[1] = m_combiner.work[threadId].functions.RGBop3[1];
            f[2] = m_combiner.work[threadId].functions.RGBop3[2];
        }
    }
    
    // Alpha
    f = CombinerMapOutputSelectArray(threadId, m_combiner.setup.stage[n].mapoutAlpha.abOutput);
    if (f) {
        f[3] = m_combiner.work[threadId].functions.Aop1;
    }
    
    f = CombinerMapOutputSelectArray(threadId, m_combiner.setup.stage[n].mapoutAlpha.cdOutput);
    if (f) {
        f[3] = m_combiner.work[threadId].functions.Aop2;
    }
    
    f = CombinerMapOutputSelectArray(threadId, m_combiner.setup.stage[n].mapoutAlpha.sumOutput);
    if (f) {
        f[3] = m_combiner.work[threadId].functions.Aop3;
    }
}

void GeForce3::MapFinalCombinerInput(int threadId)
{
    int c, d, i;
    float* pv;
    
    // E
    pv = CombinerMapInputSelectArray(threadId, m_combiner.setup.final.mapinRGB.eInput);
    c = m_combiner.setup.final.mapinRGB.eComponent * 3;
    i = m_combiner.setup.final.mapinRGB.eComponent ^ 1;
    
    for (d = 0; d < 3; d++) {
        m_combiner.work[threadId].variables.E[d] = pv[c];
        c += i;
    }
    
    CombinerMapInputFunctionArray(m_combiner.setup.final.mapinRGB.eMapping, 
                               m_combiner.work[threadId].variables.E);
    
    // F
    pv = CombinerMapInputSelectArray(threadId, m_combiner.setup.final.mapinRGB.fInput);
    c = m_combiner.setup.final.mapinRGB.fComponent * 3;
    i = m_combiner.setup.final.mapinRGB.fComponent ^ 1;
    
    for (d = 0; d < 3; d++) {
        m_combiner.work[threadId].variables.F[d] = pv[c];
        c += i;
    }
    
    CombinerMapInputFunctionArray(m_combiner.setup.final.mapinRGB.fMapping, 
                               m_combiner.work[threadId].variables.F);
    
    // EF = E * F
    m_combiner.work[threadId].variables.EF[0] = m_combiner.work[threadId].variables.E[0] * 
                                             m_combiner.work[threadId].variables.F[0];
    m_combiner.work[threadId].variables.EF[1] = m_combiner.work[threadId].variables.E[1] * 
                                             m_combiner.work[threadId].variables.F[1];
    m_combiner.work[threadId].variables.EF[2] = m_combiner.work[threadId].variables.E[2] * 
                                             m_combiner.work[threadId].variables.F[2];
    
    // SumClamp = clamp(spare0 + secondary_color)
    m_combiner.work[threadId].variables.sumClamp[0] = std::max(0.0f, m_combiner.work[threadId].registers.spare0[0]) + 
                                                  std::max(0.0f, m_combiner.work[threadId].registers.secondaryColor[0]);
    m_combiner.work[threadId].variables.sumClamp[1] = std::max(0.0f, m_combiner.work[threadId].registers.spare0[1]) + 
                                                  std::max(0.0f, m_combiner.work[threadId].registers.secondaryColor[1]);
    m_combiner.work[threadId].variables.sumClamp[2] = std::max(0.0f, m_combiner.work[threadId].registers.spare0[2]) + 
                                                  std::max(0.0f, m_combiner.work[threadId].registers.secondaryColor[2]);
    
    if (m_combiner.setup.final.colorSumClamp) {
        m_combiner.work[threadId].variables.sumClamp[0] = std::min(m_combiner.work[threadId].variables.sumClamp[0], 1.0f);
        m_combiner.work[threadId].variables.sumClamp[1] = std::min(m_combiner.work[threadId].variables.sumClamp[1], 1.0f);
        m_combiner.work[threadId].variables.sumClamp[2] = std::min(m_combiner.work[threadId].variables.sumClamp[2], 1.0f);
    }
    
    // A
    pv = CombinerMapInputSelectArray(threadId, m_combiner.setup.final.mapinRGB.aInput);
    c = m_combiner.setup.final.mapinRGB.aComponent * 3;
    i = m_combiner.setup.final.mapinRGB.aComponent ^ 1;
    
    for (d = 0; d < 3; d++) {
        m_combiner.work[threadId].variables.A[d] = pv[c];
        c += i;
    }
    
    CombinerMapInputFunctionArray(m_combiner.setup.final.mapinRGB.aMapping, 
                               m_combiner.work[threadId].variables.A);
    
    // B
    pv = CombinerMapInputSelectArray(threadId, m_combiner.setup.final.mapinRGB.bInput);
    c = m_combiner.setup.final.mapinRGB.bComponent * 3;
    i = m_combiner.setup.final.mapinRGB.bComponent ^ 1;
    
    for (d = 0; d < 3; d++) {
        m_combiner.work[threadId].variables.B[d] = pv[c];
        c += i;
    }
    
    CombinerMapInputFunctionArray(m_combiner.setup.final.mapinRGB.bMapping, 
                               m_combiner.work[threadId].variables.B);
    
    // C
    pv = CombinerMapInputSelectArray(threadId, m_combiner.setup.final.mapinRGB.cInput);
    c = m_combiner.setup.final.mapinRGB.cComponent * 3;
    i = m_combiner.setup.final.mapinRGB.cComponent ^ 1;
    
    for (d = 0; d < 3; d++) {
        m_combiner.work[threadId].variables.C[d] = pv[c];
        c += i;
    }
    
    CombinerMapInputFunctionArray(m_combiner.setup.final.mapinRGB.cMapping, 
                               m_combiner.work[threadId].variables.C);
    
    // D
    pv = CombinerMapInputSelectArray(threadId, m_combiner.setup.final.mapinRGB.dInput);
    c = m_combiner.setup.final.mapinRGB.dComponent * 3;
    i = m_combiner.setup.final.mapinRGB.dComponent ^ 1;
    
    for (d = 0; d < 3; d++) {
        m_combiner.work[threadId].variables.D[d] = pv[c];
        c += i;
    }
    
    CombinerMapInputFunctionArray(m_combiner.setup.final.mapinRGB.dMapping, 
                               m_combiner.work[threadId].variables.D);
    
    // G (alpha)
    m_combiner.work[threadId].variables.G = CombinerMapInputSelect(
        threadId, 
        m_combiner.setup.final.mapinAlpha.gInput, 
        2 + m_combiner.setup.final.mapinAlpha.gComponent
    );
    
    m_combiner.work[threadId].variables.G = CombinerMapInputFunction(
        m_combiner.setup.final.mapinAlpha.gMapping,
        m_combiner.work[threadId].variables.G
    );
}

void GeForce3::FinalCombinerOutput(int threadId)
{
    // Final combiner formula:
    // RGB = A * B + (1-A) * C + D
    // A = G
    
    // RGB calculation
    m_combiner.work[threadId].output[0] = m_combiner.work[threadId].variables.A[0] * 
                                       m_combiner.work[threadId].variables.B[0] + 
                                       (1.0f - m_combiner.work[threadId].variables.A[0]) * 
                                       m_combiner.work[threadId].variables.C[0] + 
                                       m_combiner.work[threadId].variables.D[0];
                                       
    m_combiner.work[threadId].output[1] = m_combiner.work[threadId].variables.A[1] * 
                                       m_combiner.work[threadId].variables.B[1] + 
                                       (1.0f - m_combiner.work[threadId].variables.A[1]) * 
                                       m_combiner.work[threadId].variables.C[1] + 
                                       m_combiner.work[threadId].variables.D[1];
                                       
    m_combiner.work[threadId].output[2] = m_combiner.work[threadId].variables.A[2] * 
                                       m_combiner.work[threadId].variables.B[2] + 
                                       (1.0f - m_combiner.work[threadId].variables.A[2]) * 
                                       m_combiner.work[threadId].variables.C[2] + 
                                       m_combiner.work[threadId].variables.D[2];
    
    // Clamp output
    m_combiner.work[threadId].output[0] = std::min(m_combiner.work[threadId].output[0], 2.0f);
    m_combiner.work[threadId].output[1] = std::min(m_combiner.work[threadId].output[1], 2.0f);
    m_combiner.work[threadId].output[2] = std::min(m_combiner.work[threadId].output[2], 2.0f);
    
    // Alpha comes directly from G
    m_combiner.work[threadId].output[3] = m_combiner.work[threadId].variables.G;
}

void GeForce3::CombinerFunction_AB(int threadId, float result[4])
{
    result[0] = m_combiner.work[threadId].variables.A[0] * m_combiner.work[threadId].variables.B[0];
    result[1] = m_combiner.work[threadId].variables.A[1] * m_combiner.work[threadId].variables.B[1];
    result[2] = m_combiner.work[threadId].variables.A[2] * m_combiner.work[threadId].variables.B[2];
    result[3] = m_combiner.work[threadId].variables.A[3] * m_combiner.work[threadId].variables.B[3];
}

void GeForce3::CombinerFunction_AdotB(int threadId, float result[4])
{
    // Dot product of A and B
    result[0] = m_combiner.work[threadId].variables.A[0] * m_combiner.work[threadId].variables.B[0] + 
               m_combiner.work[threadId].variables.A[1] * m_combiner.work[threadId].variables.B[1] + 
               m_combiner.work[threadId].variables.A[2] * m_combiner.work[threadId].variables.B[2];
               
    result[1] = result[2] = result[0];
    result[3] = m_combiner.work[threadId].variables.A[3] * m_combiner.work[threadId].variables.B[3];
}

void GeForce3::CombinerFunction_CD(int threadId, float result[4])
{
    result[0] = m_combiner.work[threadId].variables.C[0] * m_combiner.work[threadId].variables.D[0];
    result[1] = m_combiner.work[threadId].variables.C[1] * m_combiner.work[threadId].variables.D[1];
    result[2] = m_combiner.work[threadId].variables.C[2] * m_combiner.work[threadId].variables.D[2];
    result[3] = m_combiner.work[threadId].variables.C[3] * m_combiner.work[threadId].variables.D[3];
}

void GeForce3::CombinerFunction_CdotD(int threadId, float result[4])
{
    // Dot product of C and D
    result[0] = m_combiner.work[threadId].variables.C[0] * m_combiner.work[threadId].variables.D[0] + 
               m_combiner.work[threadId].variables.C[1] * m_combiner.work[threadId].variables.D[1] + 
               m_combiner.work[threadId].variables.C[2] * m_combiner.work[threadId].variables.D[2];
               
    result[1] = result[2] = result[0];
    result[3] = m_combiner.work[threadId].variables.C[3] * m_combiner.work[threadId].variables.D[3];
}

void GeForce3::CombinerFunction_ABmuxCD(int threadId, float result[4])
{
    if (m_combiner.work[threadId].registers.spare0[3] >= 0.5f) {
        CombinerFunction_AB(threadId, result);
    }
    else {
        CombinerFunction_CD(threadId, result);
    }
}

void GeForce3::CombinerFunction_ABsumCD(int threadId, float result[4])
{
    result[0] = m_combiner.work[threadId].variables.A[0] * m_combiner.work[threadId].variables.B[0] + 
               m_combiner.work[threadId].variables.C[0] * m_combiner.work[threadId].variables.D[0];
               
    result[1] = m_combiner.work[threadId].variables.A[1] * m_combiner.work[threadId].variables.B[1] + 
               m_combiner.work[threadId].variables.C[1] * m_combiner.work[threadId].variables.D[1];
               
    result[2] = m_combiner.work[threadId].variables.A[2] * m_combiner.work[threadId].variables.B[2] + 
               m_combiner.work[threadId].variables.C[2] * m_combiner.work[threadId].variables.D[2];
               
    result[3] = m_combiner.work[threadId].variables.A[3] * m_combiner.work[threadId].variables.B[3] + 
               m_combiner.work[threadId].variables.C[3] * m_combiner.work[threadId].variables.D[3];
}

void GeForce3::ComputeRGBOutputs(int threadId, int stageNumber)
{
    int n = stageNumber;
    int m;
    float bias, scale;
    
    // Select bias and scale
    if (m_combiner.setup.stage[n].mapoutRGB.bias)
        bias = -0.5f;
    else
        bias = 0.0f;
        
    switch (m_combiner.setup.stage[n].mapoutRGB.scale) {
        case 0:
        default:
            scale = 1.0f;
            break;
        case 1:
            scale = 2.0f;
            break;
        case 2:
            scale = 4.0f;
            break;
        case 3:
            scale = 0.5f;
            break;
    }
    
    // First
    if (m_combiner.setup.stage[n].mapoutRGB.abDotProduct) {
        m = 1;
        CombinerFunction_AdotB(threadId, m_combiner.work[threadId].functions.RGBop1);
    }
    else {
        m = 0;
        CombinerFunction_AB(threadId, m_combiner.work[threadId].functions.RGBop1);
    }
    
    m_combiner.work[threadId].functions.RGBop1[0] = std::clamp((m_combiner.work[threadId].functions.RGBop1[0] + bias) * scale, -1.0f, 1.0f);
    m_combiner.work[threadId].functions.RGBop1[1] = std::clamp((m_combiner.work[threadId].functions.RGBop1[1] + bias) * scale, -1.0f, 1.0f);
    m_combiner.work[threadId].functions.RGBop1[2] = std::clamp((m_combiner.work[threadId].functions.RGBop1[2] + bias) * scale, -1.0f, 1.0f);
    
    // Second
    if (m_combiner.setup.stage[n].mapoutRGB.cdDotProduct) {
        m = m | 1;
        CombinerFunction_CdotD(threadId, m_combiner.work[threadId].functions.RGBop2);
    }
    else
        CombinerFunction_CD(threadId, m_combiner.work[threadId].functions.RGBop2);
        
    m_combiner.work[threadId].functions.RGBop2[0] = std::clamp((m_combiner.work[threadId].functions.RGBop2[0] + bias) * scale, -1.0f, 1.0f);
    m_combiner.work[threadId].functions.RGBop2[1] = std::clamp((m_combiner.work[threadId].functions.RGBop2[1] + bias) * scale, -1.0f, 1.0f);
    m_combiner.work[threadId].functions.RGBop2[2] = std::clamp((m_combiner.work[threadId].functions.RGBop2[2] + bias) * scale, -1.0f, 1.0f);
    
    // Third
    if (m == 0) {
        if (m_combiner.setup.stage[n].mapoutRGB.muxsum)
            CombinerFunction_ABmuxCD(threadId, m_combiner.work[threadId].functions.RGBop3);
        else
            CombinerFunction_ABsumCD(threadId, m_combiner.work[threadId].functions.RGBop3);
            
        m_combiner.work[threadId].functions.RGBop3[0] = std::clamp((m_combiner.work[threadId].functions.RGBop3[0] + bias) * scale, -1.0f, 1.0f);
        m_combiner.work[threadId].functions.RGBop3[1] = std::clamp((m_combiner.work[threadId].functions.RGBop3[1] + bias) * scale, -1.0f, 1.0f);
        m_combiner.work[threadId].functions.RGBop3[2] = std::clamp((m_combiner.work[threadId].functions.RGBop3[2] + bias) * scale, -1.0f, 1.0f);
    }
}

void GeForce3::ComputeAlphaOutputs(int threadId, int stageNumber)
{
    int n = stageNumber;
    float bias, scale;
    
    // Select bias and scale
    if (m_combiner.setup.stage[n].mapoutAlpha.bias)
        bias = -0.5f;
    else
        bias = 0.0f;
        
    switch (m_combiner.setup.stage[n].mapoutAlpha.scale) {
        case 0:
        default:
            scale = 1.0f;
            break;
        case 1:
            scale = 2.0f;
            break;
        case 2:
            scale = 4.0f;
            break;
        case 3:
            scale = 0.5f;
            break;
    }
    
    // First
    m_combiner.work[threadId].functions.Aop1 = m_combiner.work[threadId].variables.A[3] * 
                                            m_combiner.work[threadId].variables.B[3];
                                            
    m_combiner.work[threadId].functions.Aop1 = std::clamp((m_combiner.work[threadId].functions.Aop1 + bias) * scale, 
                                                      -1.0f, 1.0f);
    
    // Second
    m_combiner.work[threadId].functions.Aop2 = m_combiner.work[threadId].variables.C[3] * 
                                            m_combiner.work[threadId].variables.D[3];
                                            
    m_combiner.work[threadId].functions.Aop2 = std::clamp((m_combiner.work[threadId].functions.Aop2 + bias) * scale, 
                                                      -1.0f, 1.0f);
    
    // Third
    if (m_combiner.setup.stage[n].mapoutAlpha.muxsum) {
        // Multiplex between AB and CD based on spare0 alpha
        if (m_combiner.work[threadId].registers.spare0[3] >= 0.5f)
            m_combiner.work[threadId].functions.Aop3 = m_combiner.work[threadId].variables.A[3] * 
                                                    m_combiner.work[threadId].variables.B[3];
        else
            m_combiner.work[threadId].functions.Aop3 = m_combiner.work[threadId].variables.C[3] * 
                                                    m_combiner.work[threadId].variables.D[3];
    }
    else {
        // Sum AB + CD
        m_combiner.work[threadId].functions.Aop3 = m_combiner.work[threadId].variables.A[3] * 
                                                m_combiner.work[threadId].variables.B[3] + 
                                                m_combiner.work[threadId].variables.C[3] * 
                                                m_combiner.work[threadId].variables.D[3];
    }
    
    m_combiner.work[threadId].functions.Aop3 = std::clamp((m_combiner.work[threadId].functions.Aop3 + bias) * scale, 
                                                      -1.0f, 1.0f);
}

void GeForce3::WritePixel(int x, int y, uint32_t color, int z)
{
    uint8_t* addr;
    uint32_t* dest32;
    uint16_t* dest16;
    uint32_t depthAndStencil;
    int32_t frameBuffer[4], sourceBuffer[4], destBuffer[4];
    uint32_t depth, stencil, stencilValue, stencilRef;
    uint32_t depthValue;
    bool stencilPassed;
    bool depthPassed;
    
    // Range check
    if ((z > 0xFFFFFF) || (z < 0) || (x < 0))
        return;
    
    depthValue = static_cast<uint32_t>(z);
    frameBuffer[3] = frameBuffer[2] = frameBuffer[1] = frameBuffer[0] = 0;
    addr = nullptr;
    
    // Read current pixel if color mask is not fully set
    if (m_colorMask != 0xFFFFFFFF)
        addr = ReadPixel(x, y, frameBuffer);
    
    // Fetch current depth and stencil values
    if (m_depthFormat == DepthFormat::Z24S8) {
        if (((m_depthBufferPitch / 4) * y + x) >= m_depthBufferSize) {
            LOG("Bad depth buffer offset in WritePixel!\n");
            return;
        }
        
        dest32 = m_depthBuffer + (m_depthBufferPitch / 4) * y + x;
        depthAndStencil = *dest32;
        depth = depthAndStencil >> 8;
        stencil = depthAndStencil & 0xFF;
        dest16 = nullptr;
    }
    else if (m_depthFormat == DepthFormat::Z16) {
        if (((m_depthBufferPitch / 2) * y + x) >= m_depthBufferSize) {
            LOG("Bad depth buffer offset in WritePixel!\n");
            return;
        }
        
        dest16 = reinterpret_cast<uint16_t*>(m_depthBuffer) + (m_depthBufferPitch / 2) * y + x;
        depthAndStencil = *dest16;
        depth = (depthAndStencil << 8) | 0xFF;
        stencil = 0;
        dest32 = nullptr;
    }
    else {
        dest32 = nullptr;
        dest16 = nullptr;
        depth = 0xFFFFFF;
        stencil = 0;
    }
    
    // Extract color components
    sourceBuffer[3] = color >> 24;
    sourceBuffer[2] = (color >> 16) & 0xFF;
    sourceBuffer[1] = (color >> 8) & 0xFF;
    sourceBuffer[0] = color & 0xFF;
    
    // Prepare for combiner
    destBuffer[3] = destBuffer[2] = destBuffer[1] = destBuffer[0] = 0xFF;
    
    // Alpha test
    if (m_alphaTestEnabled) {
        switch (m_alphaFunc) {
            case ComparisonOp::NEVER:
                return;
                
            case ComparisonOp::ALWAYS:
            default:
                break;
                
            case ComparisonOp::LESS:
                if (sourceBuffer[3] >= m_alphaReference)
                    return;
                break;
                
            case ComparisonOp::LEQUAL:
                if (sourceBuffer[3] > m_alphaReference)
                    return;
                break;
                
            case ComparisonOp::EQUAL:
                if (sourceBuffer[3] != m_alphaReference)
                    return;
                break;
                
            case ComparisonOp::GEQUAL:
                if (sourceBuffer[3] < m_alphaReference)
                    return;
                break;
                
            case ComparisonOp::GREATER:
                if (sourceBuffer[3] <= m_alphaReference)
                    return;
                break;
                
            case ComparisonOp::NOTEQUAL:
                if (sourceBuffer[3] == m_alphaReference)
                    return;
                break;
        }
    }
    
    // Stencil test
    stencilPassed = true;
    if (m_stencilTestEnabled) {
        stencilRef = m_stencilMask & m_stencilRef;
        stencilValue = m_stencilMask & stencil;
        
        switch (m_stencilFunc) {
            case ComparisonOp::NEVER:
                stencilPassed = false;
                break;
                
            case ComparisonOp::LESS:
                if (stencilRef >= stencilValue)
                    stencilPassed = false;
                break;
                
            case ComparisonOp::EQUAL:
                if (stencilRef != stencilValue)
                    stencilPassed = false;
                break;
                
            case ComparisonOp::LEQUAL:
                if (stencilRef > stencilValue)
                    stencilPassed = false;
                break;
                
            case ComparisonOp::GREATER:
                if (stencilRef <= stencilValue)
                    stencilPassed = false;
                break;
                
            case ComparisonOp::NOTEQUAL:
                if (stencilRef == stencilValue)
                    stencilPassed = false;
                break;
                
            case ComparisonOp::GEQUAL:
                if (stencilRef < stencilValue)
                    stencilPassed = false;
                break;
                
            case ComparisonOp::ALWAYS:
            default:
                break;
        }
        
        // Apply stencil operation if stencil test failed
        if (!stencilPassed) {
            switch (m_stencilOpFail) {
                case StencilOp::ZEROOP:
                    stencil = 0;
                    break;
                    
                case StencilOp::INVERTOP:
                    stencil = stencil ^ 0xFF;
                    break;
                    
                case StencilOp::KEEP:
                default:
                    break;
                    
                case StencilOp::REPLACE:
                    stencil = m_stencilRef;
                    break;
                    
                case StencilOp::INCR:
                    if (stencil < 0xFF)
                        stencil++;
                    break;
                    
                case StencilOp::DECR:
                    if (stencil > 0)
                        stencil--;
                    break;
                    
                case StencilOp::INCR_WRAP:
                    if (stencil < 0xFF)
                        stencil++;
                    else
                        stencil = 0;
                    break;
                    
                case StencilOp::DECR_WRAP:
                    if (stencil > 0)
                        stencil--;
                    else
                        stencil = 0xFF;
                    break;
            }
            
            // Update stencil in depth buffer
            if (m_depthFormat == DepthFormat::Z24S8) {
                depthAndStencil = (depth << 8) | stencil;
                *dest32 = depthAndStencil;
            }
            else if (m_depthFormat == DepthFormat::Z16) {
                depthAndStencil = depth >> 8;
                *dest16 = static_cast<uint16_t>(depthAndStencil);
            }
            
            return;
        }
    }
    
    // Depth test
    depthPassed = true;
    if (m_depthTestEnabled) {
        switch (m_depthFunction) {
            case ComparisonOp::NEVER:
                depthPassed = false;
                break;
                
            case ComparisonOp::LESS:
                if (depthValue >= depth)
                    depthPassed = false;
                break;
                
            case ComparisonOp::EQUAL:
                if (depthValue != depth)
                    depthPassed = false;
                break;
                
            case ComparisonOp::LEQUAL:
                if (depthValue > depth)
                    depthPassed = false;
                break;
                
            case ComparisonOp::GREATER:
                if (depthValue <= depth)
                    depthPassed = false;
                break;
                
            case ComparisonOp::NOTEQUAL:
                if (depthValue == depth)
                    depthPassed = false;
                break;
                
            case ComparisonOp::GEQUAL:
                if (depthValue < depth)
                    depthPassed = false;
                break;
                
            case ComparisonOp::ALWAYS:
            default:
                break;
        }
        
        // Apply stencil operation if depth test failed
        if (!depthPassed) {
            switch (m_stencilOpZFail) {
                case StencilOp::ZEROOP:
                    stencil = 0;
                    break;
                    
                case StencilOp::INVERTOP:
                    stencil = stencil ^ 0xFF;
                    break;
                    
                case StencilOp::KEEP:
                default:
                    break;
                    
                case StencilOp::REPLACE:
                    stencil = m_stencilRef;
                    break;
                    
                case StencilOp::INCR:
                    if (stencil < 0xFF)
                        stencil++;
                    break;
                    
                case StencilOp::DECR:
                    if (stencil > 0)
                        stencil--;
                    break;
                    
                case StencilOp::INCR_WRAP:
                    if (stencil < 0xFF)
                        stencil++;
                    else
                        stencil = 0;
                    break;
                    
                case StencilOp::DECR_WRAP:
                    if (stencil > 0)
                        stencil--;
                    else
                        stencil = 0xFF;
                    break;
            }
            
            // Update stencil in depth buffer
            if (m_depthFormat == DepthFormat::Z24S8) {
                depthAndStencil = (depth << 8) | stencil;
                *dest32 = depthAndStencil;
            }
            else if (m_depthFormat == DepthFormat::Z16) {
                depthAndStencil = depth >> 8;
                *dest16 = static_cast<uint16_t>(depthAndStencil);
            }
            
            return;
        }
        
        // Apply stencil operation if depth test passed
        switch (m_stencilOpZPass) {
            case StencilOp::ZEROOP:
                stencil = 0;
                break;
                
            case StencilOp::INVERTOP:
                stencil = stencil ^ 0xFF;
                break;
                
            case StencilOp::KEEP:
            default:
                break;
                
            case StencilOp::REPLACE:
                stencil = m_stencilRef;
                break;
                
            case StencilOp::INCR:
                if (stencil < 0xFF)
                    stencil++;
                break;
                
            case StencilOp::DECR:
                if (stencil > 0)
                    stencil--;
                break;
                
            case StencilOp::INCR_WRAP:
                if (stencil < 0xFF)
                    stencil++;
                else
                    stencil = 0;
                break;
                
            case StencilOp::DECR_WRAP:
                if (stencil > 0)
                    stencil--;
                else
                    stencil = 0xFF;
                break;
        }
    }
    
    // Apply blending
    if (m_blendingEnabled && addr != nullptr) {
        int s[4], d[4];
        int blendColor[4];
        
        blendColor[3] = m_blendColor >> 24;
        blendColor[2] = (m_blendColor >> 16) & 0xFF;
        blendColor[1] = (m_blendColor >> 8) & 0xFF;
        blendColor[0] = m_blendColor & 0xFF;
        
        // Source blending factor
        switch (m_blendFuncSource) {
            case BlendFactor::ZERO:
                s[3] = s[2] = s[1] = s[0] = 0;
                break;
                
            case BlendFactor::ONE:
            default:
                s[3] = s[2] = s[1] = s[0] = 0xFF;
                break;
                
            case BlendFactor::DST_COLOR:
                s[3] = frameBuffer[3];
                s[2] = frameBuffer[2];
                s[1] = frameBuffer[1];
                s[0] = frameBuffer[0];
                break;
                
            case BlendFactor::ONE_MINUS_DST_COLOR:
                s[3] = frameBuffer[3] ^ 0xFF;
                s[2] = frameBuffer[2] ^ 0xFF;
                s[1] = frameBuffer[1] ^ 0xFF;
                s[0] = frameBuffer[0] ^ 0xFF;
                break;
                
            case BlendFactor::SRC_ALPHA:
                s[3] = s[2] = s[1] = s[0] = sourceBuffer[3];
                break;
                
            case BlendFactor::ONE_MINUS_SRC_ALPHA:
                s[3] = s[2] = s[1] = s[0] = sourceBuffer[3] ^ 0xFF;
                break;
                
            case BlendFactor::DST_ALPHA:
                s[3] = s[2] = s[1] = s[0] = frameBuffer[3];
                break;
                
            case BlendFactor::ONE_MINUS_DST_ALPHA:
                s[3] = s[2] = s[1] = s[0] = frameBuffer[3] ^ 0xFF;
                break;
                
            case BlendFactor::CONSTANT_COLOR:
                s[3] = blendColor[3];
                s[2] = blendColor[2];
                s[1] = blendColor[1];
                s[0] = blendColor[0];
                break;
                
            case BlendFactor::ONE_MINUS_CONSTANT_COLOR:
                s[3] = blendColor[3] ^ 0xFF;
                s[2] = blendColor[2] ^ 0xFF;
                s[1] = blendColor[1] ^ 0xFF;
                s[0] = blendColor[0] ^ 0xFF;
                break;
                
            case BlendFactor::CONSTANT_ALPHA:
                s[3] = s[2] = s[1] = s[0] = blendColor[3];
                break;
                
            case BlendFactor::ONE_MINUS_CONSTANT_ALPHA:
                s[3] = s[2] = s[1] = s[0] = blendColor[3] ^ 0xFF;
                break;
                
            case BlendFactor::SRC_ALPHA_SATURATE:
                s[3] = 0xFF;
                if (sourceBuffer[3] < (frameBuffer[3] ^ 0xFF))
                    s[2] = sourceBuffer[3];
                else
                    s[2] = frameBuffer[3];
                s[1] = s[0] = s[2];
                break;
        }
        
        // Destination blending factor
        switch (m_blendFuncDest) {
            case BlendFactor::ZERO:
            default:
                d[3] = d[2] = d[1] = d[0] = 0;
                break;
                
            case BlendFactor::ONE:
                d[3] = d[2] = d[1] = d[0] = 0xFF;
                break;
                
            case BlendFactor::SRC_COLOR:
                d[3] = sourceBuffer[3];
                d[2] = sourceBuffer[2];
                d[1] = sourceBuffer[1];
                d[0] = sourceBuffer[0];
                break;
                
            case BlendFactor::ONE_MINUS_SRC_COLOR:
                d[3] = sourceBuffer[3] ^ 0xFF;
                d[2] = sourceBuffer[2] ^ 0xFF;
                d[1] = sourceBuffer[1] ^ 0xFF;
                d[0] = sourceBuffer[0] ^ 0xFF;
                break;
                
            case BlendFactor::SRC_ALPHA:
                d[3] = d[2] = d[1] = d[0] = sourceBuffer[3];
                break;
                
            case BlendFactor::ONE_MINUS_SRC_ALPHA:
                d[3] = d[2] = d[1] = d[0] = sourceBuffer[3] ^ 0xFF;
                break;
                
            case BlendFactor::DST_ALPHA:
                d[3] = d[2] = d[1] = d[0] = frameBuffer[3];
                break;
                
            case BlendFactor::ONE_MINUS_DST_ALPHA:
                d[3] = d[2] = d[1] = d[0] = frameBuffer[3] ^ 0xFF;
                break;
                
            case BlendFactor::CONSTANT_COLOR:
                d[3] = blendColor[3];
                d[2] = blendColor[2];
                d[1] = blendColor[1];
                d[0] = blendColor[0];
                break;
                
            case BlendFactor::ONE_MINUS_CONSTANT_COLOR:
                d[3] = blendColor[3] ^ 0xFF;
                d[2] = blendColor[2] ^ 0xFF;
                d[1] = blendColor[1] ^ 0xFF;
                d[0] = blendColor[0] ^ 0xFF;
                break;
                
            case BlendFactor::CONSTANT_ALPHA:
                d[3] = d[2] = d[1] = d[0] = blendColor[3];
                break;
                
            case BlendFactor::ONE_MINUS_CONSTANT_ALPHA:
                d[3] = d[2] = d[1] = d[0] = blendColor[3] ^ 0xFF;
                break;
        }
        
        // Apply blend operation
        switch (m_blendEquation) {
            case BlendEquation::FUNC_ADD:
                sourceBuffer[3] = (sourceBuffer[3] * s[3] + frameBuffer[3] * d[3]) / 0xFF;
                if (sourceBuffer[3] > 0xFF)
                    sourceBuffer[3] = 0xFF;
                    
                sourceBuffer[2] = (sourceBuffer[2] * s[2] + frameBuffer[2] * d[2]) / 0xFF;
                if (sourceBuffer[2] > 0xFF)
                    sourceBuffer[2] = 0xFF;
                    
                sourceBuffer[1] = (sourceBuffer[1] * s[1] + frameBuffer[1] * d[1]) / 0xFF;
                if (sourceBuffer[1] > 0xFF)
                    sourceBuffer[1] = 0xFF;
                    
                sourceBuffer[0] = (sourceBuffer[0] * s[0] + frameBuffer[0] * d[0]) / 0xFF;
                if (sourceBuffer[0] > 0xFF)
                    sourceBuffer[0] = 0xFF;
                break;
                
            case BlendEquation::FUNC_SUBTRACT:
                sourceBuffer[3] = (sourceBuffer[3] * s[3] - frameBuffer[3] * d[3]) / 0xFF;
                if (sourceBuffer[3] < 0)
                    sourceBuffer[3] = 0;
                    
                sourceBuffer[2] = (sourceBuffer[2] * s[2] - frameBuffer[2] * d[2]) / 0xFF;
                if (sourceBuffer[2] < 0)
                    sourceBuffer[2] = 0;
                    
                sourceBuffer[1] = (sourceBuffer[1] * s[1] - frameBuffer[1] * d[1]) / 0xFF;
                if (sourceBuffer[1] < 0)
                    sourceBuffer[1] = 0;
                    
                sourceBuffer[0] = (sourceBuffer[0] * s[0] - frameBuffer[0] * d[0]) / 0xFF;
                if (sourceBuffer[0] < 0)
                    sourceBuffer[0] = 0;
                break;
                
            case BlendEquation::FUNC_REVERSE_SUBTRACT:
                sourceBuffer[3] = (frameBuffer[3] * d[3] - sourceBuffer[3] * s[3]) / 0xFF;
                if (sourceBuffer[3] < 0)
                    sourceBuffer[3] = 0;
                    
                sourceBuffer[2] = (frameBuffer[2] * d[2] - sourceBuffer[2] * s[2]) / 0xFF;
                if (sourceBuffer[2] < 0)
                    sourceBuffer[2] = 0;
                    
                sourceBuffer[1] = (frameBuffer[1] * d[1] - sourceBuffer[1] * s[1]) / 0xFF;
                if (sourceBuffer[1] < 0)
                    sourceBuffer[1] = 0;
                    
                sourceBuffer[0] = (frameBuffer[0] * d[0] - sourceBuffer[0] * s[0]) / 0xFF;
                if (sourceBuffer[0] < 0)
                    sourceBuffer[0] = 0;
                break;
                
            case BlendEquation::MIN:
                sourceBuffer[3] = std::min(sourceBuffer[3], frameBuffer[3]);
                sourceBuffer[2] = std::min(sourceBuffer[2], frameBuffer[2]);
                sourceBuffer[1] = std::min(sourceBuffer[1], frameBuffer[1]);
                sourceBuffer[0] = std::min(sourceBuffer[0], frameBuffer[0]);
                break;
                
            case BlendEquation::MAX:
                sourceBuffer[3] = std::max(sourceBuffer[3], frameBuffer[3]);
                sourceBuffer[2] = std::max(sourceBuffer[2], frameBuffer[2]);
                sourceBuffer[1] = std::max(sourceBuffer[1], frameBuffer[1]);
                sourceBuffer[0] = std::max(sourceBuffer[0], frameBuffer[0]);
                break;
        }
    }
    
    // Apply logical operation
    if (m_logicalOpEnabled && addr != nullptr) {
        switch (m_logicalOp) {
            case LogicalOp::CLEAR:
                sourceBuffer[3] = 0;
                sourceBuffer[2] = 0;
                sourceBuffer[1] = 0;
                sourceBuffer[0] = 0;
                break;
                
            case LogicalOp::AND:
                sourceBuffer[3] = sourceBuffer[3] & frameBuffer[3];
                sourceBuffer[2] = sourceBuffer[2] & frameBuffer[2];
                sourceBuffer[1] = sourceBuffer[1] & frameBuffer[1];
                sourceBuffer[0] = sourceBuffer[0] & frameBuffer[0];
                break;
                
            case LogicalOp::AND_REVERSE:
                sourceBuffer[3] = sourceBuffer[3] & (frameBuffer[3] ^ 0xFF);
                sourceBuffer[2] = sourceBuffer[2] & (frameBuffer[2] ^ 0xFF);
                sourceBuffer[1] = sourceBuffer[1] & (frameBuffer[1] ^ 0xFF);
                sourceBuffer[0] = sourceBuffer[0] & (frameBuffer[0] ^ 0xFF);
                break;
                
            case LogicalOp::COPY:
            default:
                break;
                
            case LogicalOp::AND_INVERTED:
                sourceBuffer[3] = (sourceBuffer[3] ^ 0xFF) & frameBuffer[3];
                sourceBuffer[2] = (sourceBuffer[2] ^ 0xFF) & frameBuffer[2];
                sourceBuffer[1] = (sourceBuffer[1] ^ 0xFF) & frameBuffer[1];
                sourceBuffer[0] = (sourceBuffer[0] ^ 0xFF) & frameBuffer[0];
                break;
                
            case LogicalOp::NOOP:
                sourceBuffer[3] = frameBuffer[3];
                sourceBuffer[2] = frameBuffer[2];
                sourceBuffer[1] = frameBuffer[1];
                sourceBuffer[0] = frameBuffer[0];
                break;
                
            case LogicalOp::XOR:
                sourceBuffer[3] = sourceBuffer[3] ^ frameBuffer[3];
                sourceBuffer[2] = sourceBuffer[2] ^ frameBuffer[2];
                sourceBuffer[1] = sourceBuffer[1] ^ frameBuffer[1];
                sourceBuffer[0] = sourceBuffer[0] ^ frameBuffer[0];
                break;
                
            case LogicalOp::OR:
                sourceBuffer[3] = sourceBuffer[3] | frameBuffer[3];
                sourceBuffer[2] = sourceBuffer[2] | frameBuffer[2];
                sourceBuffer[1] = sourceBuffer[1] | frameBuffer[1];
                sourceBuffer[0] = sourceBuffer[0] | frameBuffer[0];
                break;
                
            case LogicalOp::NOR:
                sourceBuffer[3] = (sourceBuffer[3] | frameBuffer[3]) ^ 0xFF;
                sourceBuffer[2] = (sourceBuffer[2] | frameBuffer[2]) ^ 0xFF;
                sourceBuffer[1] = (sourceBuffer[1] | frameBuffer[1]) ^ 0xFF;
                sourceBuffer[0] = (sourceBuffer[0] | frameBuffer[0]) ^ 0xFF;
                break;
                
            case LogicalOp::EQUIV:
                sourceBuffer[3] = (sourceBuffer[3] ^ frameBuffer[3]) ^ 0xFF;
                sourceBuffer[2] = (sourceBuffer[2] ^ frameBuffer[2]) ^ 0xFF;
                sourceBuffer[1] = (sourceBuffer[1] ^ frameBuffer[1]) ^ 0xFF;
                sourceBuffer[0] = (sourceBuffer[0] ^ frameBuffer[0]) ^ 0xFF;
                break;
                
            case LogicalOp::INVERT:
                sourceBuffer[3] = frameBuffer[3] ^ 0xFF;
                sourceBuffer[2] = frameBuffer[2] ^ 0xFF;
                sourceBuffer[1] = frameBuffer[1] ^ 0xFF;
                sourceBuffer[0] = frameBuffer[0] ^ 0xFF;
                break;
                
            case LogicalOp::OR_REVERSE:
                sourceBuffer[3] = sourceBuffer[3] | (frameBuffer[3] ^ 0xFF);
                sourceBuffer[2] = sourceBuffer[2] | (frameBuffer[2] ^ 0xFF);
                sourceBuffer[1] = sourceBuffer[1] | (frameBuffer[1] ^ 0xFF);
                sourceBuffer[0] = sourceBuffer[0] | (frameBuffer[0] ^ 0xFF);
                break;
                
            case LogicalOp::COPY_INVERTED:
                sourceBuffer[3] = sourceBuffer[3] ^ 0xFF;
                sourceBuffer[2] = sourceBuffer[2] ^ 0xFF;
                sourceBuffer[1] = sourceBuffer[1] ^ 0xFF;
                sourceBuffer[0] = sourceBuffer[0] ^ 0xFF;
                break;
                
            case LogicalOp::OR_INVERTED:
                sourceBuffer[3] = (sourceBuffer[3] ^ 0xFF) | frameBuffer[3];
                sourceBuffer[2] = (sourceBuffer[2] ^ 0xFF) | frameBuffer[2];
                sourceBuffer[1] = (sourceBuffer[1] ^ 0xFF) | frameBuffer[1];
                sourceBuffer[0] = (sourceBuffer[0] ^ 0xFF) | frameBuffer[0];
                break;
                
            case LogicalOp::NAND:
                sourceBuffer[3] = (sourceBuffer[3] & frameBuffer[3]) ^ 0xFF;
                sourceBuffer[2] = (sourceBuffer[2] & frameBuffer[2]) ^ 0xFF;
                sourceBuffer[1] = (sourceBuffer[1] & frameBuffer[1]) ^ 0xFF;
                sourceBuffer[0] = (sourceBuffer[0] & frameBuffer[0]) ^ 0xFF;
                break;
                
            case LogicalOp::SET:
                sourceBuffer[3] = 0xFF;
                sourceBuffer[2] = 0xFF;
                sourceBuffer[1] = 0xFF;
                sourceBuffer[0] = 0xFF;
                break;
        }
    }
    
    // Apply color mask and write to framebuffer
    if (m_colorMask != 0) {
        uint32_t currentColor, finalColor;
        
        // Reconstruct current and new pixel colors
        currentColor = (frameBuffer[3] << 24) | (frameBuffer[2] << 16) | (frameBuffer[1] << 8) | frameBuffer[0];
        finalColor = (sourceBuffer[3] << 24) | (sourceBuffer[2] << 16) | (sourceBuffer[1] << 8) | sourceBuffer[0];
        
        // Apply color mask
        finalColor = (currentColor & ~m_colorMask) | (finalColor & m_colorMask);
        
        // Write back to frame buffer
        switch (m_colorFormat) {
            case ColorFormat::R5G6B5:
                {
                    uint16_t r5g6b5 = ((finalColor >> 8) & 0xF800) | 
                                      ((finalColor >> 5) & 0x07E0) | 
                                      ((finalColor >> 3) & 0x001F);
                    *reinterpret_cast<uint16_t*>(addr) = r5g6b5;
                }
                break;
                
            case ColorFormat::X8R8G8B8_Z8R8G8B8:
            case ColorFormat::X8R8G8B8_X8R8G8B8:
            case ColorFormat::A8R8G8B8:
                *reinterpret_cast<uint32_t*>(addr) = finalColor;
                break;
                
            case ColorFormat::B8:
                *addr = static_cast<uint8_t>(finalColor & 0xFF);
                break;
                
            default:
                return;
        }
    }
    
    // Update depth buffer if depth writing is enabled
    if (m_depthWriteEnabled) {
        depth = depthValue;
    }
    
    // Write back depth and stencil
    if (m_depthFormat == DepthFormat::Z24S8) {
        if (dest32 != nullptr) {
            depthAndStencil = (depth << 8) | stencil;
            *dest32 = depthAndStencil;
        }
    }
    else if (m_depthFormat == DepthFormat::Z16) {
        if (dest16 != nullptr) {
            depthAndStencil = depth >> 8;
            *dest16 = static_cast<uint16_t>(depthAndStencil);
        }
    }
}

uint8_t* GeForce3::ReadPixel(int x, int y, int32_t color[4])
{
    uint32_t offset;
    uint32_t pixelColor;
    uint32_t* addr32;
    uint16_t* addr16;
    uint8_t* addr8;
    
    // Calculate offset based on render target type
    if (m_renderTargetType == RenderTargetType::SWIZZLED) {
        offset = (Dilate0(x, m_dilateRenderTarget) + 
                 Dilate1(y, m_dilateRenderTarget)) * m_bytesPerPixel;
    }
    else { // LINEAR
        offset = m_renderTargetPitch * y + x * m_bytesPerPixel;
    }
    
    // Check for buffer overflow
    if (offset >= m_renderTargetSize) {
        LOG("Bad offset computed in ReadPixel!\n");
        offset = 0;
    }
    
    // Read color based on format
    switch (m_colorFormat) {
        case ColorFormat::R5G6B5:
            addr16 = reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_renderTarget) + offset);
            pixelColor = *addr16;
            color[3] = 0xFF;
            color[2] = Pal5bit((pixelColor & 0xF800) >> 11);
            color[1] = Pal6bit((pixelColor & 0x07E0) >> 5);
            color[0] = Pal5bit(pixelColor & 0x001F);
            return reinterpret_cast<uint8_t*>(addr16);
            
        case ColorFormat::X8R8G8B8_Z8R8G8B8:
        case ColorFormat::X8R8G8B8_X8R8G8B8:
            addr32 = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_renderTarget) + offset);
            pixelColor = *addr32;
            color[3] = 0xFF;
            color[2] = (pixelColor >> 16) & 0xFF;
            color[1] = (pixelColor >> 8) & 0xFF;
            color[0] = pixelColor & 0xFF;
            return reinterpret_cast<uint8_t*>(addr32);
            
        case ColorFormat::A8R8G8B8:
            addr32 = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_renderTarget) + offset);
            pixelColor = *addr32;
            color[3] = pixelColor >> 24;
            color[2] = (pixelColor >> 16) & 0xFF;
            color[1] = (pixelColor >> 8) & 0xFF;
            color[0] = pixelColor & 0xFF;
            return reinterpret_cast<uint8_t*>(addr32);
            
        case ColorFormat::B8:
            addr8 = reinterpret_cast<uint8_t*>(m_renderTarget) + offset;
            color[0] = *addr8;
            color[1] = color[2] = 0;
            color[3] = 0xFF;
            return addr8;
            
        default:
            return nullptr;
    }
}

// Texture address transformation functions
uint32_t GeForce3::Dilate0(uint32_t value, int bits)
{
    uint32_t x, m1, m2, m3;
    int a;
    
    x = value;
    for (a = 0; a < bits; a++) {
        m2 = 1 << (a << 1);
        m1 = m2 - 1;
        m3 = (~m1) << 1;
        x = (x & m1) + (x & m2) + ((x & m3) << 1);
    }
    return x;
}

uint32_t GeForce3::Dilate1(uint32_t value, int bits)
{
    uint32_t x, m1, m2, m3;
    int a;
    
    x = value;
    for (a = 0; a < bits; a++) {
        m2 = 1 << (a << 1);
        m1 = m2 - 1;
        m3 = (~m1) << 1;
        x = (x & m1) + ((x & m2) << 1) + ((x & m3) << 1);
    }
    return x;
}

void GeForce3::ComputeSuperSampleFactors()
{
    float mx = 1.0f;
    float my = 1.0f;
    
    // Calculate based on antialiasing settings
    switch (((m_antialiasControl & 1) << 2) | m_antialiasingRenderTarget)
    {
    case 0:
        mx = my = 1.0f;
        break;
    case 1:
        mx = 2.0f; 
        my = 1.0f;
        break;
    case 2:
        mx = my = 2.0f;
        break;
    case 4:
        mx = my = 1.0f;
        break;
    case 5:
        mx = 2.0f;
        my = 1.0f;
        break;
    case 6:
        mx = 2.0f;
        my = 2.0f;
        break;
    default:
        mx = my = 1.0f;
    }
    
    m_supersampleFactorX = mx;
    m_supersampleFactorY = my;
}

void GeForce3::UpdateRenderTargetSize()
{
    m_renderTargetSize = m_renderTargetPitch * (m_renderTargetLimits.bottom() + 1);
    m_depthBufferSize = m_depthBufferPitch * (m_renderTargetLimits.bottom() + 1);
}

// Direct memory access helper
uint8_t* GeForce3::DirectAccessPtr(uint32_t address)
{
    // Prevent access outside allocated memory
    if (address >= m_ramSize) {
        LOG("Error: DirectAccessPtr address %08X out of range\n", address);
        return m_ramBase;
    }
    
    return m_ramBase + address;
}

int GeForce3::Pal4bit(int val)
{
    return (val << 4) | val;
}

int GeForce3::Pal5bit(int val)
{
    return (val << 3) | (val >> 2);
}

int GeForce3::Pal6bit(int val)
{
    return (val << 2) | (val >> 4);
}

// Miscellaneous helper methods
void GeForce3::ProcessGPUCommands()
{
    // Process commands from all active channels
    for (int channel = 0; channel < 32; channel++) {
        uint32_t* dmaput = &m_channelState.channel[channel][0].regs[0x40/4];
        uint32_t* dmaget = &m_channelState.channel[channel][0].regs[0x44/4];
        
        while (*dmaget != *dmaput) {
            // Read command at DMAGET address
            uint32_t cmd = ReadDWORD(*dmaget);
            *dmaget += 4;
            
            // Determine command type
            CommandType cmdType = GetCommandType(cmd);
            
            switch (cmdType) {
                case CommandType::JUMP:
                    LOG("JUMP DMAGET %08X", *dmaget);
                    *dmaget = cmd & 0xFFFFFFFC;
                    LOG(" -> %08X\n", *dmaget);
                    break;
                    
                case CommandType::INCREASING:
                    {
                        uint32_t method = cmd & (2047 << 2); // Methods start at offset 0x100
                        uint32_t subchannel = (cmd >> 13) & 7;
                        uint32_t count = (cmd >> 18) & 2047;
                        
                        if ((method == 0) && (count == 1)) {
                            // Object binding method
                            AssignObject(channel, subchannel, ReadDWORD(*dmaget));
                            *dmaget += 4;
                        } else {
                            LOG("  subch. %d method %04x count %d\n", subchannel, method, count);
                            int ret = 0;
                            
                            while (count > 0) {
                                uint32_t data = ReadDWORD(*dmaget);
                                ExecuteMethod(channel, subchannel, method, data);
                                count--;
                                method += 4;
                                *dmaget += 4;
                                
                                if (ret != 0)
                                    break;
                            }
                            
                            if (ret != 0) {
                                // Method requested special handling (e.g. wait for vblank)
                                m_pullerWaiting = ret;
                                return;
                            }
                        }
                    }
                    break;
                    
                case CommandType::NON_INCREASING:
                    {
                        uint32_t method = cmd & (2047 << 2);
                        uint32_t subchannel = (cmd >> 13) & 7;
                        uint32_t count = (cmd >> 18) & 2047;
                        
                        if ((method == 0) && (count == 1)) {
                            // Object binding method
                            AssignObject(channel, subchannel, ReadDWORD(*dmaget));
                            *dmaget += 4;
                        } else {
                            LOG("  subch. %d method %04x count %d (non-increasing)\n", subchannel, method, count);
                            
                            while (count > 0) {
                                uint32_t data = ReadDWORD(*dmaget);
                                ExecuteMethod(channel, subchannel, method, data);
                                *dmaget += 4;
                                count--;
                            }
                        }
                    }
                    break;
                    
                case CommandType::LONG_NON_INCREASING:
                    {
                        uint32_t method = cmd & (2047 << 2);
                        uint32_t subchannel = (cmd >> 13) & 7;
                        uint32_t count = ReadDWORD(*dmaget);
                        *dmaget += 4;
                        
                        if ((method == 0) && (count == 1)) {
                            // Object binding method
                            AssignObject(channel, subchannel, ReadDWORD(*dmaget));
                            *dmaget += 4;
                        } else {
                            LOG("  subch. %d method %04x count %d (long non-increasing)\n", subchannel, method, count);
                            
                            while (count > 0) {
                                uint32_t data = ReadDWORD(*dmaget);
                                ExecuteMethod(channel, subchannel, method, data);
                                *dmaget += 4;
                                count--;
                            }
                        }
                    }
                    break;
                    
                case CommandType::INVALID:
                    LOG("  unimplemented command %08X\n", cmd);
                    break;
                    
                default:
                    LOG("  command type %d not implemented\n", static_cast<int>(cmdType));
                    break;
            }
        }
    }
}

// Callback processing
void GeForce3::OnVBlank(int state)
{
    // Update PCRTC registers
    if (state != 0) {
        // VBLANK start
        m_pcrtc[0x100/4] |= 1;     // Set VBLANK interrupt status bit
        m_pcrtc[0x808/4] |= 0x10000; // Set VBLANK indicator in PCRTC_INTR
        
        // If any vertical retrace (vblank) counters are active, decrement them
        for (int i = 0; i < 8; i++) {
            uint32_t counterReg = 0x800 + 0x04 * i;
            if (m_pcrtc[counterReg/4] & 0x80000000) {
                // Counter is active
                uint32_t count = m_pcrtc[counterReg/4] & 0x7FFFFFFF;
                if (count > 0) {
                    count--;
                    if (count == 0) {
                        // Counter reached zero, generate interrupt
                        m_pcrtc[0x808/4] |= (1 << i);
                    }
                }
                m_pcrtc[counterReg/4] = (count & 0x7FFFFFFF) | 0x80000000;
            }
        }
    } else {
        // VBLANK end
        m_pcrtc[0x100/4] &= ~1;
        m_pcrtc[0x808/4] &= ~0x10000;
    }
    
    // Process any waiting GPU commands
    if (state != 0 && m_pullerWaiting == 1) {
        m_pullerWaiting = 0;
        ProcessGPUCommands();
    }
    
    // Update interrupts
    if (UpdateInterrupts()) {
        if (m_irqCallback) {
            m_irqCallback(1); // Signal IRQ active
        }
    } else {
        if (m_irqCallback) {
            m_irqCallback(0); // Signal IRQ inactive
        }
    }
}

uint32_t GeForce3::ScreenUpdate(uint32_t* bitmap, int width, int height)
{
    if (m_displayTarget != nullptr) {
        // Copy display buffer to output bitmap
        memcpy(bitmap, m_displayTarget, width * height * sizeof(uint32_t));
        return 0;
    }
    
    return 0;
}

bool GeForce3::UpdateInterrupts()
{
    // Update CRTC interrupt status
    if (m_pcrtc[0x100/4] & m_pcrtc[0x140/4]) {
        m_pmc[0x100/4] |= 0x1000000; // Signal PCRTC interrupt active
    } else {
        m_pmc[0x100/4] &= ~0x1000000;
    }
    
    // Update PCRTC_INTR status
    if (m_pcrtc[0x808/4] & m_pcrtc[0x810/4]) {
        m_pcrtc[0x100/4] |= 0x01; // Set VBLANK interrupt pending
    }
    
    // Update PGRAPH interrupt status
    if (m_pgraph[0x100/4] & m_pgraph[0x140/4]) {
        m_pmc[0x100/4] |= 0x1000; // Signal PGRAPH interrupt active
    } else {
        m_pmc[0x100/4] &= ~0x1000;
    }
    
    // Check if any interrupt is active and enabled
    if (((m_pmc[0x100/4] & 0x7FFFFFFF) && (m_pmc[0x140/4] & 1)) || 
        ((m_pmc[0x100/4] & 0x80000000) && (m_pmc[0x140/4] & 2))) {
        return true; // Interrupt active
    }
    
    return false; // No interrupt
}

// Object management
CommandType GeForce3::GetCommandType(uint32_t word)
{
    if ((word & 0x00000003) == 0x00000002)
        return CommandType::CALL;
    if ((word & 0x00000003) == 0x00000001)
        return CommandType::JUMP;
    if ((word & 0xE0030003) == 0x40000000)
        return CommandType::NON_INCREASING;
    if ((word & 0xE0000003) == 0x20000000)
        return CommandType::OLD_JUMP;
    if ((word & 0xFFFF0003) == 0x00030000)
        return CommandType::LONG_NON_INCREASING;
    if ((word & 0xFFFFFFFF) == 0x00020000)
        return CommandType::RETURN;
    if ((word & 0xFFFF0003) == 0x00010000)
        return CommandType::SLI_CONDITIONAL;
    if ((word & 0xE0030003) == 0x00000000)
        return CommandType::INCREASING;
        
    return CommandType::INVALID;
}

uint32_t GeForce3::GetObjectOffset(uint32_t handle)
{
    // Calculate hash to find object in RAMHT
    uint32_t h = ((((handle >> 11) ^ handle) >> 11) ^ handle) & 0x7FF;
    uint32_t o = (m_pfifo[0x210/4] & 0x1FF) << 8;
    uint32_t e = o + h * 8;
    uint32_t w;
    
    // Check if the calculated hash points to the correct entry
    if (m_ramin[e/4] != handle) {
        // Search RAMIN for the entry if hash lookup fails
        for (uint32_t aa = o/4; aa < (sizeof(m_ramin)/4); aa += 2) {
            if (m_ramin[aa] == handle) {
                e = aa * 4;
                break;
            }
        }
    }
    
    w = m_ramin[e/4 + 1];
    return (w & 0xFFFF) * 0x10;
}

void GeForce3::AssignObject(uint32_t channel, uint32_t subchannel, uint32_t address)
{
    // Get the object handle from memory
    uint32_t handle = ReadDWORD(address);
    uint32_t offset = GetObjectOffset(handle);
    
    LOG("  assign to subchannel %d object at %d in RAMIN", subchannel, offset);
    
    // Store object information in channel state
    m_channelState.channel[channel][subchannel].object.offset = offset;
    uint32_t objClass = m_ramin[offset/4] & 0xFF;
    
    LOG(" class %03X\n", objClass);
    
    m_channelState.channel[channel][subchannel].object.objclass = objClass;
}

uint32_t GeForce3::ReadDWORD(uint32_t address)
{
    // Make sure address is within RAM
    if (address >= m_ramSize) {
        LOG("Error: ReadDWORD address %08X out of range\n", address);
        return 0xFFFFFFFF;
    }
    
    return *reinterpret_cast<uint32_t*>(m_ramBase + address);
}

// Debug helpers
bool GeForce3::ToggleRegisterCombinerUsage()
{
    m_combinerEnabled = !m_combinerEnabled;
    return m_combinerEnabled;
}

bool GeForce3::ToggleWaitVBlankSupport()
{
    m_enableWaitVblank = !m_enableWaitVblank;
    return m_enableWaitVblank;
}

bool GeForce3::ToggleClippingWSupport()
{
    m_enableClippingW = !m_enableClippingW;
    return m_enableClippingW;
}

void GeForce3::SetRamBase(void* base, uint32_t size)
{
    m_ramBase = static_cast<uint8_t*>(base);
    m_ramSize = size;
}