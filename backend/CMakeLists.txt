cmake_minimum_required(VERSION 3.10)
project(ChatReactBackend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files (all cpp except mains)
file(GLOB SOURCES
    src/*.cpp
)

# Header files
file(GLOB HEADERS
    include/*.hpp
)

# Build server executable (exclude client_main.cpp)
add_executable(chat_server
    src/server_main.cpp
    src/server.cpp
    src/client.cpp
    src/message.cpp
    src/config.cpp
    src/utils.cpp
    ${HEADERS}
)

# Build client executable (exclude server_main.cpp)
add_executable(chat_client
    src/client_main.cpp
    src/client.cpp
    src/message.cpp
    src/config.cpp
    src/utils.cpp
    ${HEADERS}
)

# Link nlohmann_json if installed via apt
find_package(nlohmann_json 3.2.0 REQUIRED)
target_link_libraries(chat_server PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(chat_client PRIVATE nlohmann_json::nlohmann_json)
