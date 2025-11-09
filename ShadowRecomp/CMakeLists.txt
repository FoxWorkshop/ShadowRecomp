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

if (WIN32)
    set(SHADOW_RECOMP_OS_CXX_SOURCES
        "os/windows/process_win32.cpp"
    )
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(SHADOW_RECOMP_OS_CXX_SOURCES
    )
endif()

set(SHADOW_RECOMP_CPU_CXX_SOURCES
    "cpu/guest_thread.cpp"
)

set(SHADOW_RECOMP_GPU_CXX_SOURCES
    "gpu/video.cpp"
    "gpu/vertexdeclaration.cpp"
    "gpu/swizzle.cpp"
    "gpu/FixedFunction.cpp"
    "gpu/renderstate.cpp"
    "gpu/rhi/plume_vulkan.cpp"
    "gpu/imgui/imgui_common.cpp"
    "gpu/imgui/imgui_font_builder.cpp"
    "gpu/imgui/imgui_snapshot.cpp"
)

if (SHADOW_RECOMP_D3D12)
    list(APPEND SHADOW_RECOMP_GPU_CXX_SOURCES
        "gpu/rhi/plume_d3d12.cpp"
    )
endif()

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
    "ui/imgui_utils.cpp"
)

set(SHADOW_RECOMP_THIRDPARTY_SOURCES
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/imgui/backends/imgui_impl_sdl2.cpp"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/imgui/imgui.cpp"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/imgui/imgui_demo.cpp"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/imgui/imgui_draw.cpp"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/imgui/imgui_tables.cpp"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/imgui/imgui_widgets.cpp"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/imgui/misc/freetype/imgui_freetype.cpp"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/implot/implot.cpp"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/implot/implot_demo.cpp"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/implot/implot_items.cpp"
)

set(SHADOW_RECOMP_USER_CXX_SOURCES
    "user/config.cpp"
    "user/paths.cpp"
)

set(SHADOW_RECOMP_THIRDPARTY_INCLUDES
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/concurrentqueue"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/imgui"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/implot"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/stb"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/unordered_dense/include"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/volk"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/Vulkan-Headers/include"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/VulkanMemoryAllocator/include"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/DirectXMath/Inc"
)

set_source_files_properties(${SHADOW_RECOMP_THIRDPARTY_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

if (SHADOW_RECOMP_D3D12)
    list(APPEND SHADOW_RECOMP_THIRDPARTY_INCLUDES "${SHADOW_RECOMP_THIRDPARTY_ROOT}/D3D12MemoryAllocator/include")
    list(APPEND SHADOW_RECOMP_THIRDPARTY_SOURCES "${SHADOW_RECOMP_THIRDPARTY_ROOT}/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp")
endif()


set(SHADOW_RECOMP_CXX_SOURCES
    "app.cpp"
    "main.cpp"
    "misc_impl.cpp"
    "sdl_listener.cpp"
    "stdafx.cpp"
    "logs.cpp"
    
    ${SHADOW_RECOMP_KERNEL_CXX_SOURCES}
    ${SHADOW_RECOMP_LOCALE_CXX_SOURCES}
    ${SHADOW_RECOMP_OS_CXX_SOURCES}
    ${SHADOW_RECOMP_CPU_CXX_SOURCES}
    ${SHADOW_RECOMP_GPU_CXX_SOURCES}
    ${SHADOW_RECOMP_APU_CXX_SOURCES}
    ${SHADOW_RECOMP_HID_CXX_SOURCES}
    ${SHADOW_RECOMP_PATCHES_CXX_SOURCES}
    ${SHADOW_RECOMP_UI_CXX_SOURCES}
    ${SHADOW_RECOMP_USER_CXX_SOURCES}
    ${SHADOW_RECOMP_THIRDPARTY_SOURCES}
)

if (WIN32)


   add_executable(ShadowRecomp ${SHADOW_RECOMP_CXX_SOURCES} "gpu/FixedFunction.h")

     # Hide console for release configurations.
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        target_link_options(ShadowRecomp PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
    endif()
else()
    add_executable(ShadowRecomp ${SHADOW_RECOMP_CXX_SOURCES})
endif()

if (SHADOW_RECOMP_D3D12)
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directx12-agility CONFIG REQUIRED)
    target_compile_definitions(ShadowRecomp PRIVATE 
        SHADOW_RECOMP_D3D12
        D3D12MA_USING_DIRECTX_HEADERS
        D3D12MA_OPTIONS16_SUPPORTED
    )
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_compile_definitions(ShadowRecomp PRIVATE SDL_VULKAN_ENABLED)
endif()

find_package(directx-dxc REQUIRED)
find_package(CURL REQUIRED)

if (SHADOW_RECOMP_D3D12)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/D3D12)
    add_custom_command(TARGET ShadowRecomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE> ${CMAKE_CURRENT_BINARY_DIR}/D3D12
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> ${CMAKE_CURRENT_BINARY_DIR}/D3D12
       COMMAND_EXPAND_LISTS
    )
    
    find_file(DIRECTX_DXIL_LIBRARY "dxil.dll")
    file(COPY ${DIRECTX_DXIL_LIBRARY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    target_link_libraries(ShadowRecomp PRIVATE
        Microsoft::DirectX-Headers 
        Microsoft::DirectX-Guids 
        Microsoft::DirectX12-Agility
        Microsoft::DirectXShaderCompiler
        Microsoft::DXIL
        dxgi
    )
endif()

file(CHMOD ${DIRECTX_DXC_TOOL} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)


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
    msdf-atlas-gen::msdf-atlas-gen
    o1heap
    XboxUtils
    SDL2::SDL2-static
    SDL2_mixer
    ShadowRecompLib
    xxHash::xxhash

)

target_include_directories(ShadowRecomp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHADOW_RECOMP_THIRDPARTY_INCLUDES}
)

target_precompile_headers(ShadowRecomp PUBLIC ${SHADOW_RECOMP_PRECOMPILED_HEADERS})

function(compile_shader FILE_PATH TARGET_NAME)
    set(FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/gpu/shader/${FILE_PATH}.hlsl)
    cmake_path(GET FILE_PATH STEM VARIABLE_NAME)
    if (SHADOW_RECOMP_D3D12)
        add_custom_command(
            OUTPUT ${FILE_PATH}.dxil.h
            COMMAND ${DIRECTX_DXC_TOOL} -T ${TARGET_NAME} -HV 2021 -all-resources-bound -Wno-ignored-attributes -Fh ${FILE_PATH}.dxil.h ${FILE_PATH} -Vn g_${VARIABLE_NAME}_dxil
            DEPENDS ${FILE_PATH}
        )
        target_sources(ShadowRecomp PRIVATE ${FILE_PATH}.dxil.h)
    endif()
    add_custom_command(
        OUTPUT ${FILE_PATH}.spirv.h
        COMMAND ${DIRECTX_DXC_TOOL} -T ${TARGET_NAME} -HV 2021 -all-resources-bound -spirv -fvk-use-dx-layout ${ARGN} -Fh ${FILE_PATH}.spirv.h ${FILE_PATH} -Vn g_${VARIABLE_NAME}_spirv
        DEPENDS ${FILE_PATH}
    )
    target_sources(ShadowRecomp PRIVATE ${FILE_PATH}.spirv.h)
endfunction()


function(compile_vertex_shader FILE_PATH)
    compile_shader(${FILE_PATH} vs_6_0 -fvk-invert-y)
endfunction()

function(compile_pixel_shader FILE_PATH)
    compile_shader(${FILE_PATH} ps_6_0)
endfunction()


compile_pixel_shader(FixedFunctionPixelShader)

compile_vertex_shader(copy_vs)
compile_pixel_shader(copy_color_ps)
compile_pixel_shader(copy_depth_ps)
compile_pixel_shader(csd_filter_ps)
compile_pixel_shader(enhanced_motion_blur_ps)
compile_pixel_shader(gaussian_blur_3x3)
compile_pixel_shader(gaussian_blur_5x5)
compile_pixel_shader(gaussian_blur_7x7)
compile_pixel_shader(gaussian_blur_9x9)
compile_pixel_shader(gamma_correction_ps)
compile_pixel_shader(imgui_ps)
compile_vertex_shader(imgui_vs)
compile_pixel_shader(resolve_msaa_color_2x)
compile_pixel_shader(resolve_msaa_color_4x)
compile_pixel_shader(resolve_msaa_color_8x)
compile_pixel_shader(resolve_msaa_depth_2x)
compile_pixel_shader(resolve_msaa_depth_4x)
compile_pixel_shader(resolve_msaa_depth_8x)

compile_vertex_shader(FixedFunctionVertexShader)
compile_vertex_shader(recomp0)
compile_vertex_shader(recomp1)
