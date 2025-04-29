cmake_minimum_required(VERSION 3.16)
project(x86Emulator VERSION 0.1.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add cmake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include utility modules
include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)
include(cmake/ExternalProjects.cmake)

# Options
option(X86EMU_BUILD_TESTS "Build tests" OFF)
option(X86EMU_USE_86BOX "Use 86Box for emulation" ON)
option(X86EMU_USE_MAME "Use MAME components" ON)
# Remove WinUAE option
# option(X86EMU_USE_WINUAE "Use WinUAE components" ON)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find required packages
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

# Set up external dependencies based on options
if(X86EMU_USE_86BOX)
    add_subdirectory(external/86box EXCLUDE_FROM_ALL)
endif()

if(X86EMU_USE_MAME)
    add_subdirectory(external/mame EXCLUDE_FROM_ALL)
endif()

# Remove WinUAE section
# if(X86EMU_USE_WINUAE)
#     add_subdirectory(external/winuae EXCLUDE_FROM_ALL)
# endif()

# Core library includes
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/86box/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/mame/src
    # Remove WinUAE include
    # ${CMAKE_CURRENT_SOURCE_DIR}/external/winuae
)

# Add source directories
add_subdirectory(src)

# Tests
if(X86EMU_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation rules
install(TARGETS x86Emulator
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install required resources
install(DIRECTORY resources/
    DESTINATION share/x86Emulator
)

# Generate package information
include(CPack)