# Define sources for core emulation
set(CORE_SOURCES
    cpu/cpu.cpp
    chipset/chipset.cpp
    io/io.cpp
    memory/memory.cpp
    emulation.cpp
    emulation_impl.cpp
)

# Add headers to source for better IDE integration
file(GLOB_RECURSE CORE_HEADERS *.h)

# Create core emulation library
add_library(x86emu_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# Set include directories
target_include_directories(x86emu_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Define preprocessor macros
target_compile_definitions(x86emu_core PRIVATE
    X86EMU_CORE_IMPLEMENTATION=1
)

# Set dependencies based on enabled options
if(X86EMU_USE_86BOX)
    target_link_libraries(x86emu_core PRIVATE x86emu_86box)
endif()

if(X86EMU_USE_MAME)
    target_link_libraries(x86emu_core PRIVATE x86emu_mame)
endif()

# Remove WinUAE dependency
# if(X86EMU_USE_WINUAE)
#     target_link_libraries(x86emu_core PRIVATE x86emu_winuae)
# endif()

# Set compiler flags
x86emu_set_compiler_flags(x86emu_core)

# Enable LTO for release builds
x86emu_enable_lto(x86emu_core)