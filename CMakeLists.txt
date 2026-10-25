# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.20)

if(NOT DEFINED ENV{VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT is not defined!")
endif()

include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

if(VCPKG_TARGET_TRIPLET MATCHES "static")
    set(VCPKG_APPLOCAL_DEPS OFF)
endif()

set(SHADOW_RECOMP_THIRDPARTY_ROOT ${CMAKE_SOURCE_DIR}/thirdparty)
set(SHADOW_RECOMP_TOOLS_ROOT ${CMAKE_SOURCE_DIR}/tools)
set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF)
set(VCPKG_APPLOCAL_DEPS OFF)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo,Release>:ProgramDatabase>>")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project ("ShadowRecomp-ALL")

# Ship symbols with release builds (for now)
if (MSVC)
    # To generate PDB needs DEBUG, but doing so disables REF, ICF (re-enabling).
    add_link_options("$<$<CONFIG:Release>:/DEBUG;/OPT:REF;/OPT:ICF>")
endif()

if (CMAKE_OSX_ARCHITECTURES)
    set(SHADOW_RECOMP_ARCHITECTURE ${CMAKE_OSX_ARCHITECTURES})
elseif(CMAKE_SYSTEM_PROCESSOR)
    set(SHADOW_RECOMP_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
else()
    set(SHADOW_RECOMP_ARCHITECTURE ${CMAKE_HOST_SYSTEM_PROCESSOR})
endif()
string(TOLOWER "${SHADOW_RECOMP_ARCHITECTURE}" SHADOW_RECOMP_ARCHITECTURE)
message(STATUS "Detected architecture: ${SHADOW_RECOMP_ARCHITECTURE}")

if (SHADOW_RECOMP_ARCHITECTURE STREQUAL "x86_64" OR SHADOW_RECOMP_ARCHITECTURE STREQUAL "amd64")
    # Target Sandy Bridge for all projects
    add_compile_options(
        -march=sandybridge
    )
endif()


add_subdirectory(${SHADOW_RECOMP_THIRDPARTY_ROOT})
add_subdirectory(${SHADOW_RECOMP_TOOLS_ROOT})

# Include sub-projects.
add_subdirectory("ShadowRecompLib")
add_subdirectory ("ShadowRecomp")