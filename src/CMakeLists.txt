# Add subdirectories first
if(X86EMU_USE_86BOX)
    add_subdirectory(86box)
endif()

if(X86EMU_USE_MAME)
    add_subdirectory(mame)
endif()

# Remove WinUAE subdirectory
# if(X86EMU_USE_WINUAE)
#     add_subdirectory(winuae)
# endif()

add_subdirectory(core)
add_subdirectory(devices)
add_subdirectory(gui)

# Define sources for main application
set(X86EMU_SOURCES
    main.cpp
)

# Add the application executable
add_executable(x86Emulator ${X86EMU_SOURCES})

# Qt settings
set_target_properties(x86Emulator PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# Define dependencies list based on enabled options
set(X86EMU_DEPENDENCIES "")

if(X86EMU_USE_86BOX)
    list(APPEND X86EMU_DEPENDENCIES x86emu_86box)
endif()

if(X86EMU_USE_MAME)
    list(APPEND X86EMU_DEPENDENCIES x86emu_mame)
endif()

# Remove WinUAE dependency
# if(X86EMU_USE_WINUAE)
#     list(APPEND X86EMU_DEPENDENCIES x86emu_winuae)
# endif()

# Link libraries
target_link_libraries(x86Emulator
    PRIVATE x86emu_core
    PRIVATE x86emu_devices
    PRIVATE x86emu_gui
    PRIVATE ${X86EMU_DEPENDENCIES}
    PRIVATE Qt5::Core
    PRIVATE Qt5::Gui
    PRIVATE Qt5::Widgets
)

# Resources
target_sources(x86Emulator PRIVATE
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

# Set compiler flags
x86emu_set_compiler_flags(x86Emulator)

# Enable LTO for release builds
x86emu_enable_lto(x86Emulator)