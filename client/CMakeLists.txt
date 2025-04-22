cmake_minimum_required(VERSION 3.10)
project(FileClient C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Add source files for client
set(CLIENT_SOURCES
    threaded_client.c
    client_utils.c
)

# Create client executable
add_executable(threaded_client ${CLIENT_SOURCES})

# Installation rules
install(TARGETS threaded_client DESTINATION bin) 