# External library management

# Function to fetch and configure external repositories
function(x86emu_add_dependency name git_repo git_tag)
    include(FetchContent)
    FetchContent_Declare(
        ${name}
        GIT_REPOSITORY ${git_repo}
        GIT_TAG ${git_tag}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(${name})
endfunction()

# Function to handle Git submodules
function(x86emu_init_submodule path)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${path}/.git")
        find_package(Git REQUIRED)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${path}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_RESULT
        )
        if(NOT GIT_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to initialize submodule at ${path}")
        endif()
    endif()
endfunction()

# Initialize required submodules if not using FetchContent
if(X86EMU_USE_86BOX AND NOT DEFINED FETCHCONTENT_BASE_DIR)
    x86emu_init_submodule(external/86box)
endif()

if(X86EMU_USE_MAME AND NOT DEFINED FETCHCONTENT_BASE_DIR)
    x86emu_init_submodule(external/mame)
endif()

# Remove WinUAE section
# if(X86EMU_USE_WINUAE AND NOT DEFINED FETCHCONTENT_BASE_DIR)
#     x86emu_init_submodule(external/winuae)
# endif()