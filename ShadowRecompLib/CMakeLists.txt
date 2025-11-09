project("ShadowRecompLib")

add_compile_options(
    -fno-strict-aliasing
)

if (WIN32)
    add_compile_options(/fp:strict)
else()
    add_compile_options(-ffp-model=strict)
endif()

target_compile_definitions(XboxRecomp PRIVATE 
    XBOX_RECOMP_CONFIG_FILE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/config/jumpTable.toml\"
    XBOX_RECOMP_HEADER_FILE_PATH=\"${SHADOW_RECOMP_TOOLS_ROOT}/XboxRecomp/XboxUtils/x86_context.h\")

set(SHADOW_RECOMP_X86_RECOMPILED_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/x86/x86_config.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/x86/x86_context.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/x86/x86_func_mapping.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/x86/x86_recomp_shared.h"
)

foreach(i RANGE 0 117)
    list(APPEND SHADOW_RECOMP_X86_RECOMPILED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/x86/x86_recomp.${i}.cpp")
endforeach()

add_custom_command(
    OUTPUT 
        ${SHADOW_RECOMP_X86_RECOMPILED_SOURCES}
    COMMAND 
        $<TARGET_FILE:XboxRecomp>
    DEPENDS 
        "${CMAKE_CURRENT_SOURCE_DIR}/private/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/config/recompConfig.toml"
)


file(GLOB XBOX_RECOMP_SOURCES 
    "${XBOX_RECOMP_ROOT}/*.h"
    "${XBOX_RECOMP_ROOT}/*.cpp"
)

add_library(ShadowRecompLib 
    ${SHADOW_RECOMP_X86_RECOMPILED_SOURCES}
)

target_include_directories(ShadowRecompLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(ShadowRecompLib PUBLIC "x86/x86_recomp_shared.h")