# Builds XboxRecomp2 from src.

# Builds the XboxRecomp2 CLI.
#
# Arguments:
# - out_executable: the output binary path.
function(xboxrecomp2_build_from_source out_executable)
    find_program(SHADOW_RECOMP_CARGO cargo REQUIRED)
    set(recompiler_root "${SHADOW_RECOMP_TOOLS_ROOT}/XboxRecomp2")

    # Ensure CMake re-runs.
    file(GLOB_RECURSE recompiler_sources CONFIGURE_DEPENDS
        "${recompiler_root}/src/*.rs"
        "${recompiler_root}/src/*.h"
        "${recompiler_root}/src/Cargo.toml"
        "${recompiler_root}/src/*/Cargo.toml"
    )

    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        ${recompiler_sources}
        "${recompiler_root}/Cargo.toml" "${recompiler_root}/Cargo.lock"
    )

    # Sewer sez: Keep this path short.
    # On Windows, paths are limited to 260 (MAX_PATH) due to skill issue.
    #
    # So zydis, which builds with MSBuild could go kapow.
    # If the user doesn't override MAX_PATH in registry.
    set(SHADOW_RECOMP_CARGO_TARGET_DIR "${CMAKE_SOURCE_DIR}/out/xr2" CACHE PATH "Cargo target directory for XboxRecomp2")
    execute_process(
        COMMAND "${SHADOW_RECOMP_CARGO}" build --release --locked
            --manifest-path "${recompiler_root}/Cargo.toml"
            --target-dir "${SHADOW_RECOMP_CARGO_TARGET_DIR}" -p xbox-recomp2-cli
        WORKING_DIRECTORY "${recompiler_root}"
        COMMAND_ERROR_IS_FATAL ANY
    )

    set(executable "${SHADOW_RECOMP_CARGO_TARGET_DIR}/release/xboxrecomp2")
    if(CMAKE_HOST_WIN32) # Confusing naming lol.
        string(APPEND executable ".exe")
    endif()

    set(${out_executable} "${executable}" PARENT_SCOPE)
endfunction()
