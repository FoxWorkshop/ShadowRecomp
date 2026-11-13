# Vendored from https://github.com/discord/discord-rpc @ 963aa9f3e5ce81a4682c6ca3d136cddda614db33 (MIT).
# Uses rapidjson from ../rapidjson.

add_library(discord-rpc STATIC
    src/discord_rpc.cpp
    src/rpc_connection.cpp
    src/serialization.cpp
)

target_include_directories(discord-rpc
    PUBLIC include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../rapidjson/include
)

if (WIN32)
    target_sources(discord-rpc PRIVATE src/connection_win.cpp src/discord_register_win.cpp)
    target_compile_definitions(discord-rpc PRIVATE DISCORD_WINDOWS)
    target_link_libraries(discord-rpc PRIVATE psapi advapi32)
else()
    target_sources(discord-rpc PRIVATE src/connection_unix.cpp src/discord_register_linux.cpp)
    target_compile_definitions(discord-rpc PRIVATE DISCORD_LINUX)
    find_package(Threads REQUIRED)
    target_link_libraries(discord-rpc PRIVATE Threads::Threads)
endif()
