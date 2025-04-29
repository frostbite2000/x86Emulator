include(ExternalProject)

# 86Box external project
if(X86EMU_USE_86BOX)
    # Configure 86Box build
    ExternalProject_Add(
        86box_external
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/86box
        CMAKE_ARGS
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/external/86box/install
            -DBUILD_SHARED_LIBS=OFF
            -DCPACK_BINARY_NSIS=OFF
            -DOLD_DYNAREC=ON
            -DDYNAMIC_OPENAL=OFF
            -DUSE_QT=OFF      # We'll provide our own Qt integration
            -DUSE_FLUIDSYNTH=OFF
            -DENABLE_DISCORD=OFF
        BUILD_BYPRODUCTS
            ${CMAKE_BINARY_DIR}/external/86box/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}86box${CMAKE_STATIC_LIBRARY_SUFFIX}
        INSTALL_DIR ${CMAKE_BINARY_DIR}/external/86box/install
    )
    
    # Create imported target for 86Box
    add_library(86box_lib STATIC IMPORTED)
    set_target_properties(86box_lib PROPERTIES
        IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/external/86box/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}86box${CMAKE_STATIC_LIBRARY_SUFFIX}
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/external/86box/install/include
    )
    add_dependencies(86box_lib 86box_external)
endif()

# MAME external project
if(X86EMU_USE_MAME)
    # We only need specific MAME components, particularly the i386 CPU core
    ExternalProject_Add(
        mame_i386_external
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/mame
        CONFIGURE_COMMAND ""  # Use MAME's build system directly
        BUILD_COMMAND 
            ${CMAKE_COMMAND} -E env SUBTARGET=i386 OSD=sdl NOWERROR=1 
            make -C ${CMAKE_CURRENT_SOURCE_DIR}/external/mame
            REGENIE=1 VERBOSE=1 -j${CMAKE_BUILD_PARALLEL_LEVEL}
        INSTALL_COMMAND
            ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/external/mame/src/mame/cpu/i386
            ${CMAKE_BINARY_DIR}/external/mame/install/include/mame/cpu/i386
        BUILD_IN_SOURCE 1
    )
    
    # Create imported target for MAME i386 CPU
    add_library(mame_i386_lib INTERFACE)
    target_include_directories(mame_i386_lib INTERFACE
        ${CMAKE_BINARY_DIR}/external/mame/install/include
    )
    add_dependencies(mame_i386_lib mame_i386_external)
endif()

# Remove WinUAE external project
# if(X86EMU_USE_WINUAE)
#     # WinUAE typically uses Visual Studio projects, so we need special handling
#     if(WIN32)
#         ExternalProject_Add(
#             winuae_external
#             SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/winuae
#             CONFIGURE_COMMAND ""
#             BUILD_COMMAND
#                 ${CMAKE_COMMAND} -E env
#                 msbuild ${CMAKE_CURRENT_SOURCE_DIR}/external/winuae/winuae.vcxproj
#                 /p:Configuration=Release /p:Platform=x64
#             INSTALL_COMMAND ""  # We'll handle installation manually
#             BUILD_IN_SOURCE 1
#         )
#     else()
#         # For non-Windows platforms, extract just the components we need
#         ExternalProject_Add(
#             winuae_external
#             SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/winuae
#             CONFIGURE_COMMAND ""
#             BUILD_COMMAND ""  # We'll just use the source files directly
#             INSTALL_COMMAND ""
#         )
#     endif()
# endif()