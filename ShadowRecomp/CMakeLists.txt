project("ShadowRecomp")

if (WIN32)
    option(SHADOW_RECOMP_D3D12 "Add D3D12 support for rendering" ON)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    option(SHADOW_RECOMP_FLATPAK "Configure the build for Flatpak compatibility." OFF)
endif()

function(BIN2C)
    cmake_parse_arguments(BIN2C_ARGS "" "TARGET_OBJ;SOURCE_FILE;DEST_FILE;ARRAY_NAME;COMPRESSION_TYPE" "" ${ARGN})

    if(NOT BIN2C_ARGS_TARGET_OBJ)
        message(FATAL_ERROR "TARGET_OBJ not specified.")
    endif()

    if(NOT BIN2C_ARGS_SOURCE_FILE)
        message(FATAL_ERROR "SOURCE_FILE not specified.")
    endif()

    if(NOT BIN2C_ARGS_DEST_FILE)
        set(BIN2C_ARGS_DEST_FILE "${BIN2C_ARGS_SOURCE_FILE}")
    endif()

    if(NOT BIN2C_ARGS_COMPRESSION_TYPE)
        set(BIN2C_ARGS_COMPRESSION_TYPE "none")
    endif()

    add_custom_command(OUTPUT "${BIN2C_ARGS_DEST_FILE}.c"
        COMMAND $<TARGET_FILE:file_to_c> "${BIN2C_ARGS_SOURCE_FILE}" "${BIN2C_ARGS_ARRAY_NAME}" "${BIN2C_ARGS_COMPRESSION_TYPE}" "${BIN2C_ARGS_DEST_FILE}.c" "${BIN2C_ARGS_DEST_FILE}.h"
        DEPENDS "${BIN2C_ARGS_SOURCE_FILE}"
        BYPRODUCTS "${BIN2C_ARGS_DEST_FILE}.h"
        COMMENT "Generating binary header for ${BIN2C_ARGS_SOURCE_FILE}..."
    )

    set_source_files_properties(${BIN2C_ARGS_DEST_FILE}.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    target_sources(${BIN2C_ARGS_TARGET_OBJ} PRIVATE ${BIN2C_ARGS_DEST_FILE}.c)
endfunction()

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


set(SHADOW_RECOMP_LOCALE_CXX_SOURCES
    "locale/achievement_locale.cpp"
    "locale/config_locale.cpp"
    "locale/locale.cpp"
)

if (WIN32)
    set(SHADOW_RECOMP_OS_CXX_SOURCES
        "os/win32/logger_win32.cpp"
        "os/win32/process_win32.cpp"
        "os/win32/user_win32.cpp"
        "os/win32/version_win32.cpp"

    )
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(SHADOW_RECOMP_OS_CXX_SOURCES
        "os/linux/logger_linux.cpp"
        "os/linux/process_linux.cpp"
        "os/linux/user_linux.cpp"
    )
endif()


set(SHADOW_RECOMP_CPU_CXX_SOURCES
    "cpu/guest_thread.cpp"
)

set(SHADOW_RECOMP_GPU_CXX_SOURCES
    "gpu/video.cpp"
    "gpu/vertexshader.cpp"
    "gpu/vertexdeclaration.cpp"
    "gpu/pixelshader.cpp"
    "gpu/pushbuffer.cpp"
    "gpu/swizzle.cpp"
    "gpu/FixedFunction.cpp"
    "gpu/renderstate.cpp"
    "gpu/imgui/imgui_common.cpp"
    "gpu/imgui/imgui_font_builder.cpp"
    "gpu/imgui/imgui_snapshot.cpp"
)


set(SHADOW_RECOMP_APU_CXX_SOURCES
    "audio/audio.cpp"
    "audio/driver/sdl2_driver.cpp"
    "audio/embedded_player.cpp"
)

set(SHADOW_RECOMP_HID_CXX_SOURCES
    "input/input.cpp"
)

set(SHADOW_RECOMP_PATCHES_CXX_SOURCES
    "patches/start_patches.cpp"
    "patches/bloom_patches.cpp"
    "patches/multi_core_patches.cpp"
    "patches/aspect_ratio_patches.cpp"
    "patches/fps_patches.cpp"
    "patches/pause_patches.cpp"
    "patches/prompt_patches.cpp"
    "patches/player_patches.cpp"
    "patches/misc_patches.cpp"
    "patches/multiplayer_mod_patches.cpp"
    "patches/reloaded_mod_patches.cpp"
    "patches/objects_patches.cpp"
    "patches/titlescreen_patches.cpp"
    "patches/unix_patches.cpp"
)


set(SHADOW_RECOMP_USER_CXX_SOURCES
    "user/achievement_data.cpp"
    "user/achievement_manager.cpp"
    "user/config.cpp"
    "user/registry.cpp"
    "user/paths.cpp"
)

set(SHADOW_RECOMP_INSTALL_CXX_SOURCES
    "install/xiso_extract.cpp"
)

set(SHADOW_RECOMP_UI_CXX_SOURCES
    "ui/achievement_menu.cpp"
    "ui/achievement_overlay.cpp"
    "ui/game_window.cpp"
    "ui/imgui_utils.cpp"
    "ui/button_guide.cpp"
    "ui/fader.cpp"
    "ui/options_menu.cpp"
    "ui/message_window.cpp"
    "ui/menu_common.cpp"
    "ui/installer_wizard.cpp"
)

set(SHADOW_RECOMP_MOD_CXX_SOURCES
    "mod/mod_loader.cpp"
    "mod/IniFile.cpp"
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
    "${SHADOW_RECOMP_TOOLS_ROOT}/XShaderRecomp/thirdparty/smol-v/source/smolv.cpp"
)


if (NOT WIN32)
    set(SHADOW_RECOMP_COMPAT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/compat")
endif()

set(SHADOW_RECOMP_THIRDPARTY_INCLUDES
    ${SHADOW_RECOMP_COMPAT_INCLUDE}
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/concurrentqueue"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/imgui"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/implot"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/stb"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/unordered_dense/include"
    "${SHADOW_RECOMP_THIRDPARTY_ROOT}/DirectXMath/Inc"
    "${SHADOW_RECOMP_TOOLS_ROOT}/XShaderRecomp/thirdparty/smol-v/source"
)

set_source_files_properties(${SHADOW_RECOMP_THIRDPARTY_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)


set(SHADOW_RECOMP_CXX_SOURCES
    "app.cpp"
    "exports.cpp"
    "main.cpp"
    "misc_impl.cpp"
    "preload_executable.cpp"
    "sdl_listener.cpp"
    "stdafx.cpp"
    
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
    ${SHADOW_RECOMP_MOD_CXX_SOURCES}
    ${SHADOW_RECOMP_INSTALL_CXX_SOURCES}
    ${SHADOW_RECOMP_THIRDPARTY_SOURCES}
)

include("version.cmake")

set(VERSION_TXT "${PROJECT_SOURCE_DIR}/res/version.txt")

# Only show Git info and build type if not Release.
set(SHOW_GIT_INFO_AND_BUILD_TYPE 0)
if (NOT ${CMAKE_BUILD_TYPE} MATCHES "Release")
    set(SHOW_GIT_INFO_AND_BUILD_TYPE 1)
endif()

GenerateVersionSources(
    OUTPUT_DIR ${PROJECT_SOURCE_DIR}
    VERSION_TXT ${VERSION_TXT}
    H_TEMPLATE "${PROJECT_SOURCE_DIR}/res/version.h.template"
    CXX_TEMPLATE "${PROJECT_SOURCE_DIR}/res/version.cpp.template"
    BUILD_TYPE ${CMAKE_BUILD_TYPE}
    SHOW_GIT_INFO ${SHOW_GIT_INFO_AND_BUILD_TYPE}
    SHOW_BUILD_TYPE ${SHOW_GIT_INFO_AND_BUILD_TYPE}
)

if (WIN32)
    # Create binary version number for Win32 integer attributes.
    CreateVersionString(
        VERSION_TXT ${VERSION_TXT}
        OUTPUT_CSV 1
        OUTPUT_VAR WIN32_VERSION_BINARY
    )

    # Create string version number for Win32 detailed attributes.
    CreateVersionString(
        VERSION_TXT ${VERSION_TXT}
        BUILD_TYPE ${CMAKE_BUILD_TYPE}
        SHOW_GIT_INFO ${SHOW_GIT_INFO_AND_BUILD_TYPE}
        SHOW_BUILD_TYPE ${SHOW_GIT_INFO_AND_BUILD_TYPE}
        OUTPUT_VAR WIN32_VERSION_STRING
    )

    # Set Win32 icon path.
    set(WIN32_ICON_PATH "${PROJECT_SOURCE_DIR}/../ShadowRecompResources/images/game_icon.ico")

    configure_file("res/Win32_/res.rc.template" "${CMAKE_BINARY_DIR}/res.rc" @ONLY)
    add_executable(ShadowRecomp ${SHADOW_RECOMP_CXX_SOURCES} "${CMAKE_BINARY_DIR}/res.rc")

     # Hide console for release configurations.
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        target_link_options(ShadowRecomp PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
    endif()
else()
    add_executable(ShadowRecomp ${SHADOW_RECOMP_CXX_SOURCES})
endif()

if (SHADOW_RECOMP_FLATPAK)
    target_compile_definitions(ShadowRecomp PRIVATE
        "GAME_INSTALL_DIRECTORY=\"/var/data\""
    )
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

#find_package(directx-dxc REQUIRED)
find_package(CURL REQUIRED)
find_package(nfd REQUIRED)

include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(nlohmann_json)


if (SHADOW_RECOMP_D3D12)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/D3D12)
    add_custom_command(TARGET ShadowRecomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE> ${CMAKE_CURRENT_BINARY_DIR}/D3D12
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> ${CMAKE_CURRENT_BINARY_DIR}/D3D12
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectXShaderCompiler,IMPORTED_LOCATION> $<TARGET_FILE_DIR:ShadowRecomp>
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DXIL,IMPORTED_LOCATION> $<TARGET_FILE_DIR:ShadowRecomp>
       COMMAND_EXPAND_LISTS
    )
    
    find_file(DIRECTX_DXIL_LIBRARY "dxil.dll")
#    file(COPY ${DIRECTX_DXIL_LIBRARY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    target_link_libraries(ShadowRecomp PRIVATE
        Microsoft::DirectX-Headers 
        Microsoft::DirectX-Guids 
        Microsoft::DirectX12-Agility
        Microsoft::DirectXShaderCompiler
        Microsoft::DXIL
        dxgi
    )
endif()

if (WIN32)
    target_link_libraries(ShadowRecomp PRIVATE
        comctl32
        dwmapi
        ntdll
        Shcore
        Synchronization
        winmm
        windowsapp
    )
endif()

target_link_libraries(ShadowRecomp PRIVATE
    fmt::fmt
    libzstd_static
    msdf-atlas-gen::msdf-atlas-gen
    o1heap
    XboxUtils
    SDL2::SDL2-static
    SDL2_mixer
    tomlplusplus::tomlplusplus
    ShadowRecompLib
    xxHash::xxhash
    plume
    XShaderRecompLib
    nlohmann_json::nlohmann_json
    nfd::nfd
)


target_compile_definitions(XShaderRecompLib PRIVATE SHADOW_RECOMP)

target_include_directories(ShadowRecomp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHADOW_RECOMP_THIRDPARTY_INCLUDES}
)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(X11 REQUIRED)
    target_include_directories(ShadowRecomp PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(ShadowRecomp PRIVATE ${X11_LIBRARIES})
    find_package(Freetype REQUIRED)
    target_link_libraries(ShadowRecomp PRIVATE Freetype::Freetype)

    set_target_properties(ShadowRecomp PROPERTIES BUILD_RPATH "$ORIGIN")
    add_custom_command(TARGET ShadowRecomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectXShaderCompiler,IMPORTED_LOCATION> $<TARGET_FILE_DIR:ShadowRecomp>
        COMMAND_EXPAND_LISTS
    )
endif()

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

compile_vertex_shader(FixedFunctionVertexShader)
compile_pixel_shader(FixedFunctionPixelShader)
compile_vertex_shader(PassthroughVertexShader)

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


compile_vertex_shader(R2D_vs0)
compile_vertex_shader(R2D_vs1)

function(generate_aggregate_header INPUT_DIRECTORY OUTPUT_FILE)
    get_filename_component(ABS_OUTPUT_FILE "${OUTPUT_FILE}" ABSOLUTE)
    file(GLOB_RECURSE HEADER_FILES "${INPUT_DIRECTORY}/*.h")
    set(HEADER_CONTENT "#pragma once\n\n")

    foreach(HEADER_FILE IN LISTS HEADER_FILES)
        get_filename_component(ABS_HEADER_FILE "${HEADER_FILE}" ABSOLUTE)
        if (ABS_HEADER_FILE STREQUAL ABS_OUTPUT_FILE)
            continue()
        endif()
        file(RELATIVE_PATH RELATIVE_HEADER_FILE "${INPUT_DIRECTORY}" "${HEADER_FILE}")
        string(APPEND HEADER_CONTENT "#include \"${RELATIVE_HEADER_FILE}\"\n")
    endforeach()

    if (EXISTS "${OUTPUT_FILE}")
        file(READ "${OUTPUT_FILE}" EXISTING_CONTENT)
        if (EXISTING_CONTENT STREQUAL HEADER_CONTENT)
            return()
        endif()
    endif()

    file(WRITE "${OUTPUT_FILE}" "${HEADER_CONTENT}")
endfunction()

generate_aggregate_header(
    "${CMAKE_CURRENT_SOURCE_DIR}/api"
    "${CMAKE_CURRENT_SOURCE_DIR}/api/Shadow.h"
)

set(RESOURCES_SOURCE_PATH "${PROJECT_SOURCE_DIR}/../ShadowRecompResources")
set(RESOURCES_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/res")

## Miscellaneous ##
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/font/im_font_atlas.bin" DEST_FILE "${RESOURCES_OUTPUT_PATH}/font/im_font_atlas.bin" ARRAY_NAME "g_im_font_atlas" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/font/im_font_atlas.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/font/im_font_atlas.png" ARRAY_NAME "g_im_font_atlas_texture" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/gamemenu.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/gamemenu.png" ARRAY_NAME "g_gameMenuText" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/Options.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/Options.png" ARRAY_NAME "g_optionsText" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/Options2.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/Options2.png" ARRAY_NAME "g_optionsIcons" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/TitleBg.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/TitleBg.png" ARRAY_NAME "g_options_bg" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/controllerBtn.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/controllerBtn.png" ARRAY_NAME "g_controller" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/achievementBG.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/achievementBG.png" ARRAY_NAME "g_achievementBG" COMPRESSION_TYPE "zstd")

BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/arrow.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/arrow.png" ARRAY_NAME "g_arrow" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/achievementBtn.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/achievementBtn.png" ARRAY_NAME "g_achievementBtn" COMPRESSION_TYPE "zstd")

## Game Icon ##
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/game_icon.bmp" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/game_icon.bmp" ARRAY_NAME "g_game_icon")

## Audio #
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/music/installer.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/music/installer.ogg" ARRAY_NAME "g_installer_music")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/sounds/sys_menu_ok.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/sounds/sys_menu_ok.ogg" ARRAY_NAME "g_sys_menu_ok")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/sounds/sys_menu_cursor.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/sounds/sys_menu_cursor.ogg" ARRAY_NAME "g_sys_menu_cursor")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/sounds/sys_menu_invalid.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/sounds/sys_menu_invalid.ogg" ARRAY_NAME "g_sys_menu_invalid")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/sounds/sys_menu_cancel.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/sounds/sys_menu_cancel.ogg" ARRAY_NAME "g_sys_menu_cancel")

## Achievements #
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/achievements/pure_hero.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/pure_hero.png" ARRAY_NAME "g_purehero" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/achievements/neutral_hero.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/neutral_hero.png" ARRAY_NAME "g_neutralhero" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/achievements/PlaceHolder.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/PlaceHolder.png" ARRAY_NAME "g_placeholder" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ ShadowRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/achievements/pure_dark.png" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/pure_dark.png" ARRAY_NAME "g_puredark" COMPRESSION_TYPE "zstd")