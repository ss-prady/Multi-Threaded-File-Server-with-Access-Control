cmake_minimum_required(VERSION 3.10)
project(FileClient C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Add source files for client
set(CLIENT_SOURCES
    main.c                # Main client logic
    client_utils.c        # Helper functions
)

# Create client executable with new name
add_executable(file_client ${CLIENT_SOURCES})

# Find all .txt files in the current directory
file(GLOB TXT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.txt")

# Copy each .txt file to the build directory
foreach(TXT_FILE ${TXT_FILES})
    get_filename_component(FILENAME ${TXT_FILE} NAME)
    configure_file(${TXT_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME} COPYONLY)
endforeach()

# Installation rules
install(TARGETS file_client DESTINATION bin)

# Install all .txt files
install(FILES ${TXT_FILES} DESTINATION bin)
