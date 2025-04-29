#include "emulation_impl.h"
#include "globals.h"
#include "../86box/box86_factory.h"
#include "../86box/box86_module.h"
#include "../mame/mame_factory.h"
#include "../mame/mame_module.h"
// Remove WinUAE include
// #include "../winuae/winuae_factory.h"
// #include "../winuae/winuae_module.h"
#include <stdexcept>

namespace x86emu {

EmulationImpl::EmulationImpl()
    : m_initialized(false)
    , m_paused(true)
    , m_cpuBackend(CPUBackend::MAME)  // Default to MAME CPU
    , m_machineType(MachineType::GENERIC_PENTIUM)
{
}

EmulationImpl::~EmulationImpl()
{
    Shutdown();
}

bool EmulationImpl::Initialize()
{
    if (m_initialized) {
        return true;
    }
    
    // Initialize the CPU backend
    switch (m_cpuBackend) {
        case CPUBackend::BOX86:
            // Initialize 86Box module
            if (!box86::InitializeBox86Module()) {
                return false;
            }
            
            // Create core components using 86Box adapters
            m_cpu = box86::Box86Factory::CreateCPU(CPU::MODEL_80586); // Pentium
            m_memory = box86::Box86Factory::CreateMemory();
            m_io = box86::Box86Factory::CreateIO();
            break;
            
        case CPUBackend::MAME:
            // Initialize MAME module
            if (!mame::InitializeMameModule()) {
                return false;
            }
            
            // Create core components using MAME adapters for CPU
            m_cpu = mame::MameFactory::CreateCPU(CPU::MODEL_80586); // Pentium
            m_memory = box86::Box86Factory::CreateMemory();
            m_io = box86::Box86Factory::CreateIO();
            break;
            
        default:
            return false;
    }
    
    // Set up global interfaces for adapters to use
    SetGlobalMemory(m_memory.get());
    SetGlobalIO(m_io.get());
    
    // Configure the system based on machine type
    bool deviceSuccess = false;
    
    switch (m_machineType) {
        case MachineType::SIS_630_SYSTEM:
            deviceSuccess = ConfigureSiS630System();
            break;
            
        case MachineType::GENERIC_PENTIUM:
        default:
            deviceSuccess = InitializeGenericPentiumSystem();
            break;
    }
    
    if (!deviceSuccess) {
        return false;
    }
    
    // Initialize network manager with a loopback handler for testing
    auto loopbackHandler = std::make_shared<LoopbackNetworkHandler>();
    NetworkManager::Instance().SetNetworkHandler(loopbackHandler);
    
    // Reset all components
    Reset();
    
    m_initialized = true;
    return true;
}

void EmulationImpl::Shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    // Clear device manager
    DeviceManager::Instance().Clear();
    
    // Clear network manager
    NetworkManager::Instance().SetNetworkHandler(nullptr);
    
    // Clear CPU, memory, and I/O interfaces
    m_cpu.reset();
    m_memory.reset();
    m_io.reset();
    
    m_initialized = false;
}

void EmulationImpl::Reset()
{
    // Reset CPU, memory, and I/O
    if (m_cpu) {
        m_cpu->Reset();
    }
    
    if (m_memory) {
        m_memory->Reset();
    }
    
    if (m_io) {
        m_io->Reset();
    }
    
    // Reset all devices
    DeviceManager::Instance().ResetAll();
    
    m_paused = true;
}

bool EmulationImpl::RunFrame()
{
    if (!m_initialized || m_paused) {
        return false;
    }
    
    // Calculate frame time in microseconds (16.67ms for 60Hz)
    static const uint32_t frameTimeUs = 16667;
    
    try {
        // Run a fixed number of CPU cycles representing a frame
        // For 60 Hz operation at 25 MHz, that's about 416,667 cycles per frame
        static const int cyclesPerFrame = 416667;
        
        // Execute CPU cycles
        m_cpu->Execute(cyclesPerFrame);
        
        // Update all devices
        DeviceManager::Instance().UpdateAll(frameTimeUs);
        
        // Special handling for SiS 630 VGA if present
        auto videoDevice = DeviceManager::Instance().GetDeviceByType<VideoDevice>();
        auto sis630VGA = dynamic_cast<SiS630VGA*>(videoDevice);
        
        if (sis630VGA) {
            // The SiS 630 VGA device will handle its own rendering internally
            // We just need to ensure it's updated with the current frame time
            sis630VGA->Update(frameTimeUs);
        }
        
        return true;
    } catch (const std::exception& e) {
        // Log the error and return false to indicate failure
        return false;
    }
}

void EmulationImpl::Pause(bool pause)
{
    m_paused = pause;
}

bool EmulationImpl::IsPaused() const
{
    return m_paused;
}

bool EmulationImpl::IsInitialized() const
{
    return m_initialized;
}

bool EmulationImpl::LoadConfiguration(const std::string& filename)
{
    // Load configuration from file
    // This is a placeholder implementation
    return true;
}

bool EmulationImpl::SaveConfiguration(const std::string& filename)
{
    // Save configuration to file
    // This is a placeholder implementation
    return true;
}

CPU* EmulationImpl::GetCPU() const
{
    return m_cpu.get();
}

Memory* EmulationImpl::GetMemory() const
{
    return m_memory.get();
}

IO* EmulationImpl::GetIO() const
{
    return m_io.get();
}

void EmulationImpl::SetCPUBackend(CPUBackend backend)
{
    if (m_initialized) {
        throw std::runtime_error("Cannot change CPU backend after initialization");
    }
    
    m_cpuBackend = backend;
}

CPUBackend EmulationImpl::GetCPUBackend() const
{
    return m_cpuBackend;
}

bool EmulationImpl::SetMachineType(MachineType type)
{
    if (m_initialized) {
        // Can't change machine type after initialization
        return false;
    }
    
    m_machineType = type;
    return true;
}

MachineType EmulationImpl::GetMachineType() const
{
    return m_machineType;
}

bool EmulationImpl::InitializeGenericPentiumSystem()
{
    using namespace devices;
    
    // Create chipset devices
    auto chipset = DeviceFactory::CreateChipset86Box("i440bx", DeviceFactory::CHIPSET_INTEL_440BX);
    if (!DeviceManager::Instance().RegisterDevice(std::move(chipset))) {
        return false;
    }
    
    auto sio = DeviceFactory::CreateChipset86Box("vt82c686b", DeviceFactory::CHIPSET_VIA_VT82C686B);
    if (!DeviceManager::Instance().RegisterDevice(std::move(sio))) {
        return false;
    }
    
    // Create IDE controllers
    auto ide_primary = DeviceFactory::CreateIDEController86Box("ide0", DeviceFactory::IDE_INTEL_PIIX, true);
    if (!DeviceManager::Instance().RegisterDevice(std::move(ide_primary))) {
        return false;
    }
    
    auto ide_secondary = DeviceFactory::CreateIDEController86Box("ide1", DeviceFactory::IDE_INTEL_PIIX, false);
    if (!DeviceManager::Instance().RegisterDevice(std::move(ide_secondary))) {
        return false;
    }
    
    // Create network adapter
    auto nic = DeviceFactory::CreateNetworkAdapter86Box("nic0", DeviceFactory::NET_RTL8139);
    if (!DeviceManager::Instance().RegisterDevice(std::move(nic))) {
        return false;
    }
    
    // Create VGA adapter
    auto vga = DeviceFactory::CreateVGA86Box("vga0", DeviceFactory::VGA_S3_VIRGE);
    if (!DeviceManager::Instance().RegisterDevice(std::move(vga))) {
        return false;
    }
    
    // Create floppy drives
    auto floppy0 = DeviceFactory::CreateFloppy86Box("floppy0", 0);
    if (!DeviceManager::Instance().RegisterDevice(std::move(floppy0))) {
        return false;
    }
    
    auto floppy1 = DeviceFactory::CreateFloppy86Box("floppy1", 1);
    if (!DeviceManager::Instance().RegisterDevice(std::move(floppy1))) {
        return false;
    }
    
    // Create hard drive
    auto hdd0 = DeviceFactory::CreateHardDrive86Box("hdd0", 8192);  // 8GB hard drive
    if (!DeviceManager::Instance().RegisterDevice(std::move(hdd0))) {
        return false;
    }
    
    // Create CDROM drive
    auto cdrom0 = DeviceFactory::CreateCDROM86Box("cdrom0", 0);
    if (!DeviceManager::Instance().RegisterDevice(std::move(cdrom0))) {
        return false;
    }
    
    // Create sound card
    auto sb = DeviceFactory::CreateSoundBlasterMAME("sb0", DeviceFactory::SOUND_SB16);
    if (!DeviceManager::Instance().RegisterDevice(std::move(sb))) {
        return false;
    }
    
    // Initialize all devices
    if (!DeviceManager::Instance().InitializeAll()) {
        return false;
    }
    
    // Connect devices where needed
    auto storageController = static_cast<StorageController*>(DeviceManager::Instance().GetDevice("ide0"));
    auto hardDrive = static_cast<StorageDevice*>(DeviceManager::Instance().GetDevice("hdd0"));
    auto cdromDrive = static_cast<StorageDevice*>(DeviceManager::Instance().GetDevice("cdrom0"));
    
    if (storageController && hardDrive && cdromDrive) {
        // Attach hard drive to primary IDE controller (master)
        storageController->AttachDevice(0, "hdd0");
        
        // Attach CDROM to primary IDE controller (slave)
        storageController->AttachDevice(1, "cdrom0");
    }
    
    // Register the network adapter with the network manager
    auto rtl8139 = static_cast<RTL8139*>(DeviceManager::Instance().GetDevice("nic0"));
    if (rtl8139) {
        NetworkManager::Instance().RegisterDevice("nic0", rtl8139);
        rtl8139->Connect("default");
    }
    
    return true;
}

bool EmulationImpl::ConfigureSiS630System()
{
    using namespace devices;
    
    // Create SiS 630 chipset with integrated VGA
    if (!DeviceFactory::CreateSiS630WithVGA("sis630", "sis630_vga")) {
        return false;
    }
    
    // Create IDE controllers (SiS 630 has integrated IDE)
    auto ide_primary = DeviceFactory::CreateIDEController86Box("ide0", DeviceFactory::IDE_GENERIC, true);
    if (!DeviceManager::Instance().RegisterDevice(std::move(ide_primary))) {
        return false;
    }
    
    auto ide_secondary = DeviceFactory::CreateIDEController86Box("ide1", DeviceFactory::IDE_GENERIC, false);
    if (!DeviceManager::Instance().RegisterDevice(std::move(ide_secondary))) {
        return false;
    }
    
    // Create network adapter - use RTL8139 which was common with SiS 630 systems
    auto rtl8139_pci = DeviceFactory::CreateRTL8139PCI("rtl8139");
    if (!DeviceManager::Instance().RegisterDevice(std::move(rtl8139_pci))) {
        return false;
    }
    
    // Register the RTL8139 network device with the network manager
    auto rtl8139 = static_cast<RTL8139PCI*>(DeviceManager::Instance().GetDevice("rtl8139"))->GetAdapter();
    NetworkManager::Instance().RegisterDevice("rtl8139", rtl8139);
    
    // Create floppy drive
    auto floppy0 = DeviceFactory::CreateFloppy86Box("floppy0", 0);
    if (!DeviceManager::Instance().RegisterDevice(std::move(floppy0))) {
        return false;
    }
    
    // Create hard drive
    auto hdd0 = DeviceFactory::CreateHardDrive86Box("hdd0", 4096);  // 4GB hard drive
    if (!DeviceManager::Instance().RegisterDevice(std::move(hdd0))) {
        return false;
    }
    
    // Create CDROM drive
    auto cdrom0 = DeviceFactory::CreateCDROM86Box("cdrom0", 0);
    if (!DeviceManager::Instance().RegisterDevice(std::move(cdrom0))) {
        return false;
    }
    
    // Create sound card - typical AC97 codec for SiS 630
    auto ac97 = DeviceFactory::CreateAC97MAME("ac97_0");
    if (!DeviceManager::Instance().RegisterDevice(std::move(ac97))) {
        return false;
    }
    
    // Initialize all devices
    if (!DeviceManager::Instance().InitializeAll()) {
        return false;
    }
    
    // Connect devices where needed
    auto storageController = static_cast<StorageController*>(DeviceManager::Instance().GetDevice("ide0"));
    auto hardDrive = static_cast<StorageDevice*>(DeviceManager::Instance().GetDevice("hdd0"));
    auto cdromDrive = static_cast<StorageDevice*>(DeviceManager::Instance().GetDevice("cdrom0"));
    
    if (storageController && hardDrive && cdromDrive) {
        // Attach hard drive to primary IDE controller (master)
        storageController->AttachDevice(0, "hdd0");
        
        // Attach CDROM to primary IDE controller (slave)
        storageController->AttachDevice(1, "cdrom0");
    }
    
    // Connect the RTL8139 to the network
    rtl8139->Connect("default");
    
    return true;
}

} // namespace x86emu