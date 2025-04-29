# Define sources for network device implementations
set(NETWORK_SOURCES
    ne2000_86box.cpp
    3c905c_86box.cpp
    rtl8139.cpp
)

# Add headers for better IDE integration
file(GLOB NETWORK_HEADERS *.h)

# Create network devices library
add_library(x86emu_devices_network STATIC ${NETWORK_SOURCES} ${NETWORK_HEADERS})

# Set include directories
target_include_directories(x86emu_devices_network
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..
    PRIVATE ${CMAKE_BINARY_DIR}/external/86box/install/include
)

# Link with 86Box library
if(TARGET 86box_lib)
    target_link_libraries(x86emu_devices_network
        PRIVATE 86box_lib
    )
endif()

# Set compiler flags
x86emu_set_compiler_flags(x86emu_devices_network)