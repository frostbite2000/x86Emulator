#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <algorithm>
#include <limits>
#include <cmath>
#include "device.h"
#include "pci.h"
#include "rasterizer.h"

// GeForce3 (NV20) GPU emulation
class GeForce3 : public PCIDevice
{
public:
    GeForce3();
    ~GeForce3();
    
    // PCIDevice interface
    virtual bool Initialize() override;
    virtual void Reset() override;
    virtual uint32_t ReadConfig(uint8_t reg, int size) override;
    virtual void WriteConfig(uint8_t reg, uint32_t value, int size) override;
    virtual uint32_t ReadMemory(uint32_t address, int size) override;
    virtual void WriteMemory(uint32_t address, uint32_t value, int size) override;
    
    // GPU control
    void SetRamBase(void* base, uint32_t size);
    void OnVBlank(int state);
    uint32_t ScreenUpdate(uint32_t* bitmap, int width, int height);
    void SetIRQCallback(std::function<void(int state)> callback) { m_irqCallback = callback; }
    
    // Debug helpers
    bool ToggleRegisterCombinerUsage();
    bool ToggleWaitVBlankSupport();
    bool ToggleClippingWSupport();

private:
    // VGA CRTC registers
    uint8_t m_vgaRegs[0x100];  // Extended VGA registers
    uint8_t m_vgaCRTCIndex;    // Currently selected CRTC index

    // RMA registers
    struct {
        uint32_t configA;      // RMA Config A register
        uint32_t configB;      // RMA Config B register
        uint32_t control;      // RMA Control register
        uint32_t limit;        // RMA Limit register
        uint32_t cursorConfig; // RMA Cursor config
        uint32_t cursorPos;    // RMA Cursor position
        uint32_t fifoConfig;   // RMA FIFO config
        uint32_t fifoStatus;   // RMA FIFO status
    } m_rmaRegs;

    // PCI Configuration space
    uint32_t m_pciConfig[256/4];  // Full PCI configuration space in 32-bit word

    // Vertex shader operations
    enum class ScalarOp {
        NOP = 0,
        IMV,
        RCP,
        RCC,
        RSQ,
        EXP,
        LOG,
        LIT
    };
    
    enum class VectorOp {
        NOP = 0,
        MOV,
        MUL,
        ADD,
        MAD,
        DP3,
        DPH,
        DP4,
        DST,
        MIN,
        MAX,
        SLT,
        SGE,
        ARL
    };
    
    // GPU command types
    enum class CommandType {
        CALL = 7,
        JUMP = 6,
        NON_INCREASING = 5,
        OLD_JUMP = 4,
        LONG_NON_INCREASING = 3,
        RETURN = 2,
        SLI_CONDITIONAL = 1,
        INCREASING = 0,
        INVALID = -1
    };
    
    // Primitive types
    enum class PrimitiveType {
        STOP = 0,
        POINTS = 1,
        LINES = 2,
        LINE_LOOP = 3,
        LINE_STRIP = 4,
        TRIANGLES = 5,
        TRIANGLE_STRIP = 6,
        TRIANGLE_FAN = 7,
        QUADS = 8,
        QUAD_STRIP = 9,
        POLYGON = 10
    };
    
    // Vertex attributes
    enum class VertexAttr {
        POS = 0,
        WEIGHT = 1,
        NORMAL = 2,
        COLOR0 = 3,
        COLOR1 = 4,
        FOG = 5,
        BACKCOLOR0 = 7,
        BACKCOLOR1 = 8,
        TEX0 = 9,
        TEX1 = 10,
        TEX2 = 11,
        TEX3 = 12
    };
    
    // Vertex parameter indices
    enum class VertexParameter {
        PARAM_COLOR_B = 0,
        PARAM_COLOR_G,
        PARAM_COLOR_R,
        PARAM_COLOR_A,
        PARAM_TEXTURE0_S,
        PARAM_TEXTURE0_T,
        PARAM_TEXTURE0_R,
        PARAM_TEXTURE0_Q,
        PARAM_TEXTURE1_S,
        PARAM_TEXTURE1_T,
        PARAM_TEXTURE1_R,
        PARAM_TEXTURE1_Q,
        PARAM_TEXTURE2_S,
        PARAM_TEXTURE2_T,
        PARAM_TEXTURE2_R,
        PARAM_TEXTURE2_Q,
        PARAM_TEXTURE3_S,
        PARAM_TEXTURE3_T,
        PARAM_TEXTURE3_R,
        PARAM_TEXTURE3_Q,
        PARAM_SECONDARY_COLOR_B,
        PARAM_SECONDARY_COLOR_G,
        PARAM_SECONDARY_COLOR_R,
        PARAM_SECONDARY_COLOR_A,
        PARAM_Z,
        PARAM_1W,
        ALL
    };
    
    // Texture formats
    enum class TexFormat {
        L8 = 0x0,
        I8 = 0x1,
        A1R5G5B5 = 0x2,
        A4R4G4B4 = 0x4,
        R5G6B5 = 0x5,
        A8R8G8B8 = 0x6,
        X8R8G8B8 = 0x7,
        INDEX8 = 0xb,
        DXT1 = 0xc,
        DXT3 = 0xe,
        DXT5 = 0xf,
        A1R5G5B5_RECT = 0x10,
        R5G6B5_RECT = 0x11,
        A8R8G8B8_RECT = 0x12,
        L8_RECT = 0x13,
        DSDT8_RECT = 0x17,
        A8 = 0x19,
        A8L8 = 0x1a,
        I8_RECT = 0x1b,
        A4R4G4B4_RECT = 0x1d,
        R8G8B8_RECT = 0x1e,
        A8L8_RECT = 0x20,
        Z24 = 0x2a,
        Z24_RECT = 0x2b,
        Z16 = 0x2c,
        Z16_RECT = 0x2d
    };
    
    // Render target types and formats
    enum class RenderTargetType {
        LINEAR = 1,
        SWIZZLED = 2
    };
    
    enum class DepthFormat {
        Z16 = 0x0001,
        Z24S8 = 0x0002
    };
    
    enum class ColorFormat {
        X1R5G5B5_Z1R5G5B5 = 1,
        X1R5G5B5_X1R5G5B5 = 2,
        R5G6B5 = 3,
        X8R8G8B8_Z8R8G8B8 = 4,
        X8R8G8B8_X8R8G8B8 = 5,
        X1A7R8G8B8_Z1A7R8G8B8 = 6,
        X1A7R8G8B8_X1A7R8G8B8 = 7,
        A8R8G8B8 = 8,
        B8 = 9,
        G8B8 = 10
    };
    
    // Stencil, blend, and comparison operations
    enum class StencilOp {
        ZEROOP = 0x0000,
        INVERTOP = 0x150a,
        KEEP = 0x1e00,
        REPLACE = 0x1e01,
        INCR = 0x1e02,
        DECR = 0x1e03,
        INCR_WRAP = 0x8507,
        DECR_WRAP = 0x8508
    };
    
    enum class BlendEquation {
        FUNC_ADD = 0x8006,
        MIN = 0x8007,
        MAX = 0x8008,
        FUNC_SUBTRACT = 0x800a,
        FUNC_REVERSE_SUBTRACT = 0x800b
    };
    
    enum class BlendFactor {
        ZERO = 0x0000,
        ONE = 0x0001,
        SRC_COLOR = 0x0300,
        ONE_MINUS_SRC_COLOR = 0x0301,
        SRC_ALPHA = 0x0302,
        ONE_MINUS_SRC_ALPHA = 0x0303,
        DST_ALPHA = 0x0304,
        ONE_MINUS_DST_ALPHA = 0x0305,
        DST_COLOR = 0x0306,
        ONE_MINUS_DST_COLOR = 0x0307,
        SRC_ALPHA_SATURATE = 0x0308,
        CONSTANT_COLOR = 0x8001,
        ONE_MINUS_CONSTANT_COLOR = 0x8002,
        CONSTANT_ALPHA = 0x8003,
        ONE_MINUS_CONSTANT_ALPHA = 0x8004
    };
    
    enum class ComparisonOp {
        NEVER = 0x0200,
        LESS = 0x0201,
        EQUAL = 0x0202,
        LEQUAL = 0x0203,
        GREATER = 0x0204,
        NOTEQUAL = 0x0205,
        GEQUAL = 0x0206,
        ALWAYS = 0x0207
    };
    
    enum class LogicalOp {
        CLEAR = 0x1500,
        AND = 0x1501,
        AND_REVERSE = 0x1502,
        COPY = 0x1503,
        AND_INVERTED = 0x1504,
        NOOP = 0x1505,
        XOR = 0x1506,
        OR = 0x1507,
        NOR = 0x1508,
        EQUIV = 0x1509,
        INVERT = 0x150a,
        OR_REVERSE = 0x150b,
        COPY_INVERTED = 0x150c,
        OR_INVERTED = 0x150d,
        NAND = 0x150e,
        SET = 0x150f
    };
    
    enum class CullingWinding {
        CW = 0x0900,
        CCW = 0x0901
    };
    
    enum class CullingFace {
        FRONT = 0x0404,
        BACK = 0x0405,
        FRONT_AND_BACK = 0x0408
    };
    
    // Register combiner enums
    enum class CombinerInputRegister {
        ZERO = 0,
        COLOR0,
        COLOR1,
        FOG_COLOR,
        PRIMARY_COLOR,
        SECONDARY_COLOR,
        TEXTURE0_COLOR = 8,
        TEXTURE1_COLOR,
        TEXTURE2_COLOR,
        TEXTURE3_COLOR,
        SPARE0,
        SPARE1,
        SUM_CLAMP,
        EF
    };
    
    enum class CombinerMapFunction {
        UNSIGNED_IDENTITY = 0,
        UNSIGNED_INVERT,
        EXPAND_NORMAL,
        EXPAND_NEGATE,
        HALF_BIAS_NORMAL,
        HALF_BIAS_NEGATE,
        SIGNED_IDENTITY,
        SIGNED_NEGATE
    };
    
    // Vertex data structures
    struct VertexNV {
        union {
            float fv[4];
            uint32_t iv[4];
        } attribute[16];
    };
    
    struct VertexProgramInstruction {
        uint32_t data[4];
        int modified;
        
        struct {
            int SwizzleA[4], NegateA, ParameterTypeA, TempIndexA;
            int SwizzleB[4], NegateB, ParameterTypeB, TempIndexB;
            int SwizzleC[4], NegateC, ParameterTypeC, TempIndexC;
            VectorOp VecOperation;
            ScalarOp ScaOperation;
            int OutputWriteMask, MultiplexerControl;
            int VecTempWriteMask, ScaTempWriteMask;
            int VecTempIndex, OutputIndex;
            int InputIndex;
            int SourceConstantIndex;
            int OutputSelect;
            int Usea0x;
            int EndOfProgram;
        } decoded;
    };
    
    struct VertexConstant {
        float fv[4];
        
        void SetComponent(int idx, uint32_t value) {
            union {
                uint32_t i;
                float f;
            } cnv;
            
            cnv.i = value;
            fv[idx] = cnv.f;
        }
    };
    
    struct VertexRegister {
        float fv[4];
    };
    
    // GPU state structures
    struct Rectangle {
        int x1, y1, x2, y2;
        
        Rectangle() : x1(0), y1(0), x2(0), y2(0) {}
        Rectangle(int left, int top, int right, int bottom) 
            : x1(left), y1(top), x2(right), y2(bottom) {}
        
        void set(int left, int top, int right, int bottom) {
            x1 = left;
            y1 = top;
            x2 = right;
            y2 = bottom;
        }
        
        void setx(int left, int right) {
            x1 = left;
            x2 = right;
        }
        
        void sety(int top, int bottom) {
            y1 = top;
            y2 = bottom;
        }
        
        int left() const { return x1; }
        int right() const { return x2; }
        int top() const { return y1; }
        int bottom() const { return y2; }
    };
    
    struct VertexBufferState {
        uint32_t address[16];
        int type[16];
        int stride[16];
        int words[16];
        int offset[16 + 1];
        int enabled;  // bitmask
    };
    
    struct TextureState {
        int enabled;
        int sizeS;
        int sizeT;
        int sizeR;
        int dilate;
        TexFormat format;
        bool rectangular;
        int rectanglePitch;
        void* buffer;
        int dma0;
        int dma1;
        int mode;
        int cubic;
        int noBorder;
        int dims;
        int mipmap;
        int colorKey;
        int imageField;
        int aniso;
        int mipMapMaxLOD;
        int mipMapMinLOD;
        int rectHeight;
        int rectWidth;
        int addrModeS;
        int addrModeT;
        int addrModeR;
    };
    
    struct BitBlitState {
        int format;
        uint32_t sourcePitch;
        uint32_t destinationPitch;
        uint32_t sourceAddress;
        uint32_t destinationAddress;
        int op;
        int width;
        int height;
        uint32_t sourceX;
        uint32_t sourceY;
        uint32_t destX;
        uint32_t destY;
    };
    
    // Rasterizer vertex structure
    struct NV2AVertex_t : public RasterizerVertex_t {
        double w;
    };
    
    // Register combiner structures
    struct CombinerMapIn {
        CombinerInputRegister aInput;
        int aComponent;
        CombinerMapFunction aMapping;
        CombinerInputRegister bInput;
        int bComponent;
        CombinerMapFunction bMapping;
        CombinerInputRegister cInput;
        int cComponent;
        CombinerMapFunction cMapping;
        CombinerInputRegister dInput;
        int dComponent;
        CombinerMapFunction dMapping;
    };
    
    struct CombinerMapInAlpha {
        CombinerInputRegister gInput;
        int gComponent;
        CombinerMapFunction gMapping;
    };
    
    struct CombinerMapInRGB {
        CombinerInputRegister aInput;
        int aComponent;
        CombinerMapFunction aMapping;
        CombinerInputRegister bInput;
        int bComponent;
        CombinerMapFunction bMapping;
        CombinerInputRegister cInput;
        int cComponent;
        CombinerMapFunction cMapping;
        CombinerInputRegister dInput;
        int dComponent;
        CombinerMapFunction dMapping;
        CombinerInputRegister eInput;
        int eComponent;
        CombinerMapFunction eMapping;
        CombinerInputRegister fInput;
        int fComponent;
        CombinerMapFunction fMapping;
    };
    
    struct CombinerMapOut {
        CombinerInputRegister cdOutput;
        CombinerInputRegister abOutput;
        CombinerInputRegister sumOutput;
        int cdDotProduct;
        int abDotProduct;
        int muxsum;
        int bias;
        int scale;
    };
    
    struct CombinerStage {
        float constantColor0[4];
        float constantColor1[4];
        CombinerMapIn mapinAlpha;
        CombinerMapIn mapinRGB;
        CombinerMapOut mapoutAlpha;
        CombinerMapOut mapoutRGB;
    };
    
    struct CombinerFinal {
        float constantColor0[4];
        float constantColor1[4];
        int colorSumClamp;
        CombinerMapInAlpha mapinAlpha;
        CombinerMapInRGB mapinRGB;
    };
    
    struct CombinerSetup {
        CombinerStage stage[8];
        CombinerFinal final;
        int stages;
    };
    
    struct CombinerVariables {
        float A[4];
        float B[4];
        float C[4];
        float D[4];
        float E[4];
        float F[4];
        float G;
        float EF[4];
        float sumClamp[4];
    };
    
    struct CombinerFunctions {
        float RGBop1[4];
        float RGBop2[4];
        float RGBop3[4];
        float Aop1;
        float Aop2;
        float Aop3;
    };
    
    struct CombinerRegisters {
        float primaryColor[4];
        float secondaryColor[4];
        float texture0Color[4];
        float texture1Color[4];
        float texture2Color[4];
        float texture3Color[4];
        float color0[4];
        float color1[4];
        float spare0[4];
        float spare1[4];
        float fogColor[4];
        float zero[4];
    };
    
    struct CombinerWork {
        CombinerVariables variables;
        CombinerFunctions functions;
        CombinerRegisters registers;
        float output[4];
    };
    
    struct Combiner {
        CombinerWork work[16]; // Support for multiple threads
        CombinerSetup setup;
    };
    
    // Matrix state
    struct MatrixState {
        float modelview[4][4];
        float modelviewInverse[4][4];
        float composite[4][4];
        float projection[4][4];
        float translate[4];
        float scale[4];
    };
    
    // Vertex program state
    struct VertexProgramState {
        VertexProgramInstruction instructions[256];
        VertexConstant constants[192];
        VertexRegister registers[32];
        int instructionCount;
        int uploadInstructionIndex;
        int uploadInstructionComponent;
        int startInstruction;
        int uploadParameterIndex;
        int uploadParameterComponent;
        int ip;
        int a0x;
    };
    
    // Channel state
    struct ChannelState {
        struct {
            uint32_t regs[0x80/4];
            struct {
                uint32_t offset;
                uint32_t objclass;
                uint32_t method[0x2000/4];
            } object[8];
        } channel[32];
    };
    
    // Rasterizer extent for rendering scanlines
    struct RasterizerExtent_t {
        int32_t startx;
        int32_t stopx;
        struct {
            double start;
            double dpdx;
        } param[26]; // One per vertex parameter
    };
    
    // Method handling
    void ExecuteMethod(uint32_t channelID, uint32_t subchannelID, uint32_t method, uint32_t parameter);
    void ExecuteMethodGraphics(uint32_t channelID, uint32_t subchannelID, uint32_t method, uint32_t parameter);
    void ExecuteMethodM2MF(uint32_t channelID, uint32_t subchannelID, uint32_t method, uint32_t parameter);
    void ExecuteMethodSurf2D(uint32_t channelID, uint32_t subchannelID, uint32_t method, uint32_t parameter);
    void ExecuteMethodBlit(uint32_t channelID, uint32_t subchannelID, uint32_t method, uint32_t parameter);
    
    void HandleTextureMethod(uint32_t method, uint32_t parameter);
    void HandleDrawCommand(uint32_t parameter);
    void HandleDrawIndices(int multiply, uint32_t parameter);
    void HandleDrawVerticesWithOffset(uint32_t parameter);
    int HandleDrawRawVertices(uint32_t parameter);
    void HandleRenderTargetDimensions(uint32_t parameter);
    void HandleRenderTargetHeight(uint32_t parameter);
    void HandleRenderTargetFormat(uint32_t parameter);
    void HandleFinalCombinerInputRGB(uint32_t parameter);
    void HandleFinalCombinerInputAlpha(uint32_t parameter);
    void HandleCombinerStageAlphaInput(int stage, uint32_t parameter);
    void HandleCombinerStageRGBInput(int stage, uint32_t parameter);
    
    // Vertex processing
    void DecodeVertexInstruction(int address);
    void ExecuteVertexProgram();
    bool ExecuteVertexInstruction();
    void GenerateInput(float* output, bool negate, int type, int tempIndex, const int* swizzle);
    void ComputeVectorOperation(float* output, VectorOp op, const float* a, const float* b, const float* c);
    void ComputeScalarOperation(float* output, ScalarOp op, const float* a, const float* b, const float* c);
    void AssignToRegister(int index, const float* value, int mask);
    void AssignToOutput(int index, const float* value, int mask);
    void AssignToConstant(int index, const float* value, int mask);
    
    // DMA and object management
    void ReadDMAObject(uint32_t handle, uint32_t& offset, uint32_t& size);
    void AssignObject(uint32_t channel, uint32_t subchannel, uint32_t address);
    uint32_t GetObjectOffset(uint32_t handle);
    CommandType GetCommandType(uint32_t word);
    uint32_t ReadDWORD(uint32_t address);
    
    // Vertex data handling
    void ReadVerticesFromIndices(int destination, uint32_t address, int count);
    void ReadVerticesWithOffset(int destination, int offset, int count);
    int ReadRawVertices(int destination, uint32_t address, int limit);
    void ReadVertex(uint32_t address, VertexNV& vertex, int attrib);
    void ExtractPackedFloat(uint32_t data, float& first, float& second, float& third);
    
    // Primitive assembly and rendering
    void AssemblePrimitive(int source, int count);
    void ConvertVertices(VertexNV* source, NV2AVertex_t* destination);
    uint32_t RenderTriangleClipping(const Rectangle& cliprect, NV2AVertex_t& v1, NV2AVertex_t& v2, NV2AVertex_t& v3);
    int ClipTriangleW(NV2AVertex_t vi[3], NV2AVertex_t* vo);
    uint32_t RenderTriangleCulling(const Rectangle& cliprect, NV2AVertex_t& v1, NV2AVertex_t& v2, NV2AVertex_t& v3);
    
    // Render methods (called by rasterizer)
    void RenderColor(int32_t scanline, const RasterizerExtent_t& extent, void* objectData, int threadId);
    void RenderTextureSimple(int32_t scanline, const RasterizerExtent_t& extent, void* objectData, int threadId);
    void RenderRegisterCombiners(int32_t scanline, const RasterizerExtent_t& extent, void* objectData, int threadId);
    
    // Pixel processing
    void WritePixel(int x, int y, uint32_t color, int z);
    uint8_t* ReadPixel(int x, int y, int32_t color[4]);
    uint32_t GetTexel(int texUnit, int x, int y);
    uint32_t BilinearFilter(uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11, int xFrac, int yFrac);
    
    // Color conversion helpers
    uint32_t ConvertA4R4G4B4ToARGB8(uint32_t a4r4g4b4);
    uint32_t ConvertA1R5G5B5ToARGB8(uint32_t a1r5g5b5);
    uint32_t ConvertR5G6B5ToRGB8(uint32_t r5g6b5);
    void ConvertARGB8ToFloat(uint32_t color, float reg[4]);
    uint32_t ConvertFloatToARGB8(float reg[4]);
    
    // Combiner methods
    float CombinerMapInputFunction(CombinerMapFunction code, float value);
    void CombinerMapInputFunctionArray(CombinerMapFunction code, float* data);
    void InitializeCombinerRegisters(int threadId, float rgba[7][4]);
    void InitializeCombinerStage(int threadId, int stageNumber);
    void InitializeFinalCombiner(int threadId);
    float CombinerMapInputSelect(int threadId, CombinerInputRegister code, int index);
    float* CombinerMapInputSelectArray(int threadId, CombinerInputRegister code);
    float* CombinerMapOutputSelectArray(int threadId, CombinerInputRegister code);
    void MapCombinerStageInput(int threadId, int stageNumber);
    void MapCombinerStageOutput(int threadId, int stageNumber);
    void MapFinalCombinerInput(int threadId);
    void FinalCombinerOutput(int threadId);
    
    void CombinerFunction_AB(int threadId, float result[4]);
    void CombinerFunction_AdotB(int threadId, float result[4]);
    void CombinerFunction_CD(int threadId, float result[4]);
    void CombinerFunction_CdotD(int threadId, float result[4]);
    void CombinerFunction_ABmuxCD(int threadId, float result[4]);
    void CombinerFunction_ABsumCD(int threadId, float result[4]);
    void ComputeRGBOutputs(int threadId, int stageNumber);
    void ComputeAlphaOutputs(int threadId, int stageNumber);
    
    // Texture address transformation functions
    uint32_t Dilate0(uint32_t value, int bits);
    uint32_t Dilate1(uint32_t value, int bits);
    
    // Color palette conversion helpers
    int Pal4bit(int val);
    int Pal5bit(int val);
    int Pal6bit(int val);
    
    // Miscellaneous helper methods
    void PerformBlit();
    void ProcessGPUCommands();
    uint8_t* DirectAccessPtr(uint32_t address);
    void UpdateRenderTargetSize();
    void ComputeSuperSampleFactors();
    bool UpdateInterrupts();
    
    // PCI state
    uint32_t m_fbMemBase;
    uint32_t m_mmioMemBase;
    
    // Memory and registers
    uint8_t* m_ramBase;
    uint32_t m_ramSize;
    
    uint32_t m_pfifo[0x2000/4];
    uint32_t m_pcrtc[0x1000/4];
    uint32_t m_pmc[0x1000/4];
    uint32_t m_pgraph[0x2000/4];
    uint32_t m_ramin[0x100000/4];
    
    uint32_t m_dmaOffset[13];
    uint32_t m_dmaSize[13];
    
    ChannelState m_channelState;
    std::function<void(int state)> m_irqCallback;
    
    // Render state
    Rectangle m_renderTargetLimits;
    Rectangle m_clearRect;
    uint32_t m_renderTargetPitch;
    uint32_t m_depthBufferPitch;
    uint32_t m_renderTargetSize;
    uint32_t m_depthBufferSize;
    
    int m_log2HeightRenderTarget;
    int m_log2WidthRenderTarget;
    int m_dilateRenderTarget;
    int m_antialiasingRenderTarget;
    
    RenderTargetType m_renderTargetType;
    DepthFormat m_depthFormat;
    ColorFormat m_colorFormat;
    int m_bytesPerPixel;
    
    uint32_t m_antialiasControl;
    float m_supersampleFactorX;
    float m_supersampleFactorY;
    
    uint32_t* m_renderTarget;
    uint32_t* m_depthBuffer;
    uint32_t* m_displayTarget;
    uint32_t* m_oldRenderTarget;
    
    // Vertex and primitive state
    VertexBufferState m_vertexBufferState;
    TextureState m_texture[4];
    
    uint32_t m_trianglesBfCulled;
    PrimitiveType m_primitiveType;
    uint32_t m_primitivesCount;
    uint32_t m_primitivesBatchCount;
    
    int m_indexesLeftCount;
    int m_indexesLeftFirst;
    uint32_t m_vertexIndices[1024];
    
    int m_vertexCount;
    int m_vertexFirst;
    int m_vertexAccumulated;
    
    VertexNV m_vertexSoftware[1026]; // Extra space for persistent vertex and copy
    NV2AVertex_t m_vertexXY[1026];
    VertexNV m_persistentVertexAttr;
    
    // Vertex processing state
    VertexNV* m_vertexInput;
    VertexNV* m_vertexOutput;
    VertexNV m_vertexTemp;
    
    std::function<void(int32_t, const RasterizerExtent_t&, void*, int)> m_renderCallback;
    MatrixState m_matrices;
    VertexProgramState m_vertexProgram;
    int m_vertexPipeline;
    
    // Blending and rasterization state
    uint32_t m_colorMask;
    bool m_backfaceCullingEnabled;
    CullingWinding m_backfaceCullingWinding;
    CullingFace m_backfaceCullingFace;
    
    bool m_alphaTestEnabled;
    bool m_depthTestEnabled;
    bool m_stencilTestEnabled;
    bool m_depthWriteEnabled;
    bool m_blendingEnabled;
    bool m_logicalOpEnabled;
    
    ComparisonOp m_alphaFunc;
    int m_alphaReference;
    ComparisonOp m_depthFunction;
    
    ComparisonOp m_stencilFunc;
    int m_stencilRef;
    int m_stencilMask;
    StencilOp m_stencilOpFail;
    StencilOp m_stencilOpZFail;
    StencilOp m_stencilOpZPass;
    
    BlendEquation m_blendEquation;
    BlendFactor m_blendFuncSource;
    BlendFactor m_blendFuncDest;
    uint32_t m_blendColor;
    
    LogicalOp m_logicalOp;
    uint32_t m_fogColor;
    bool m_bilinearFilter;
    
    // Blit state
    BitBlitState m_bitBlit;
    
    // Combiner state
    Combiner m_combiner;
    bool m_combinerEnabled;
    
    // Command processing
    int m_pullerWaiting;
    
    // Debug flags
    bool m_enableWaitVblank;
    bool m_enableClippingW;
    
    // Rasterizer pointer
    Rasterizer* m_rasterizer;
    
    // Dilate tables
    uint32_t m_dilated0[16][2048];
    uint32_t m_dilated1[16][2048];
    int m_dilateChose[256];
};