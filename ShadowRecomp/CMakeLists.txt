project("ShadowRecomp")

if (WIN32)
    option(SHADOW_RECOMP_D3D12 "Add D3D12 support for rendering" ON)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    option(SHADOW_RECOMP_FLATPAK "Configure the build for Flatpak compatibility." OFF)
endif()

add_compile_options(
    -fno-strict-aliasing
    -Wno-switch
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-but-set-variable
    -Wno-void-pointer-to-int-cast
    -Wno-int-to-void-pointer-cast
    -Wno-invalid-offsetof
    -Wno-null-arithmetic
    -Wno-null-conversion
    -Wno-tautological-undefined-compare
)


if (WIN32)
    add_compile_options(/fp:strict)
else()
    add_compile_options(-ffp-model=strict)
endif()

add_compile_definitions(
    SDL_MAIN_HANDLED
    _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR 
    _CRT_SECURE_NO_WARNINGS)

set(SHADOW_RECOMP_PRECOMPILED_HEADERS
    "stdafx.h"
)

set(SHADOW_RECOMP_KERNEL_CXX_SOURCES
    "kernel/imports.cpp"
    "kernel/kernel.cpp"
    "kernel/heap.cpp"
    "kernel/memory.cpp"
    "kernel/io/file_system.cpp"
)

set(SHADOW_RECOMP_CPU_CXX_SOURCES
    "cpu/guest_thread.cpp"
)

set(SHADOW_RECOMP_GPU_CXX_SOURCES
    "gpu/video.cpp"
)

set(SHADOW_RECOMP_APU_CXX_SOURCES
    "audio/audio.cpp"
    "audio/driver/sdl2_driver.cpp"
)

set(SHADOW_RECOMP_HID_CXX_SOURCES
    "input/input.cpp"
)

set(SHADOW_RECOMP_PATCHES_CXX_SOURCES
    "patches/start_patches.cpp"
)

set(SHADOW_RECOMP_UI_CXX_SOURCES
    "ui/game_window.cpp"
)

set(SHADOW_RECOMP_THIRDPARTY_INCLUDES
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/stb"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/unordered_dense/include"
)

set_source_files_properties(${SHADOW_RECOMP_THIRDPARTY_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

set(SHADOW_RECOMP_CXX_SOURCES
    "app.cpp"
    "main.cpp"
    "misc_impl.cpp"
    "sdl_listener.cpp"
    "stdafx.cpp"
    "logs.cpp"
    
    ${SHADOW_RECOMP_KERNEL_CXX_SOURCES}
    ${SHADOW_RECOMP_LOCALE_CXX_SOURCES}
    ${SHADOW_RECOMP_CPU_CXX_SOURCES}
    ${SHADOW_RECOMP_GPU_CXX_SOURCES}
    ${SHADOW_RECOMP_APU_CXX_SOURCES}
    ${SHADOW_RECOMP_HID_CXX_SOURCES}
    ${SHADOW_RECOMP_PATCHES_CXX_SOURCES}
    ${SHADOW_RECOMP_UI_CXX_SOURCES}
)

if (WIN32)


    add_executable(ShadowRecomp ${SHADOW_RECOMP_CXX_SOURCES})

    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        target_link_options(ShadowRecomp PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
    endif()
else()
    add_executable(ShadowRecomp ${SHADOW_RECOMP_CXX_SOURCES})
endif()



if (WIN32)
    target_link_libraries(ShadowRecomp PRIVATE
        comctl32
        dwmapi
        ntdll
        Shcore
        Synchronization
        winmm
    )
endif()

target_link_libraries(ShadowRecomp PRIVATE
    fmt::fmt
    o1heap
    XboxUtils
    SDL2::SDL2-static
    SDL2_mixer
    #vgmstream::vgmstream-static
    #libvgmstream
    ShadowRecompLib
)

target_include_directories(ShadowRecomp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHADOW_RECOMP_THIRDPARTY_INCLUDES}
)

target_precompile_headers(ShadowRecomp PUBLIC ${SHADOW_RECOMP_PRECOMPILED_HEADERS})

