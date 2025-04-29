# Define sources for PCI device implementations
set(PCI_SOURCES
    pci_device.cpp
    rtl8139_pci.cpp
)

# Add headers for better IDE integration
file(GLOB PCI_HEADERS *.h)

# Create PCI devices library
add_library(x86emu_devices_pci STATIC ${PCI_SOURCES} ${PCI_HEADERS})

# Set include directories
target_include_directories(x86emu_devices_pci
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Link with network devices library
target_link_libraries(x86emu_devices_pci
    PRIVATE x86emu_devices_network
)

# Set compiler flags
x86emu_set_compiler_flags(x86emu_devices_pci)