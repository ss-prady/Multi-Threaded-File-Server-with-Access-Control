cmake_minimum_required(VERSION 3.10)
project(FileServer C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Add pthread library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Add OpenSSL for cryptographic functions
find_package(OpenSSL REQUIRED)

# Add source files for main server
set(SERVER_SOURCES
    server.c
    user_auth.c
    client_handler.c
    file_handler.c
    logger.c  # Add logger.c to the sources
)

# Create main server executable
add_executable(file_server ${SERVER_SOURCES})

# Link against required libraries for server
target_link_libraries(file_server PRIVATE 
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Create user manager utility
add_executable(user_manager user_manager.c user_auth.c)

# Link against OpenSSL for user manager
target_link_libraries(user_manager PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Copy users to build directory
configure_file(users users COPYONLY)

# Create files directory in build directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/files)

# Get all files in the files directory
file(GLOB FILES_DIR_CONTENTS "${CMAKE_CURRENT_SOURCE_DIR}/files/*")

# Copy each file from the files directory to the build directory, excluding README.md
foreach(FILE ${FILES_DIR_CONTENTS})
    get_filename_component(FILENAME ${FILE} NAME)
    if(NOT ${FILENAME} STREQUAL "README.md")
        configure_file(${FILE} ${CMAKE_CURRENT_BINARY_DIR}/files/${FILENAME} COPYONLY)
    endif()
endforeach()

# Installation rules
install(TARGETS file_server user_manager DESTINATION bin)
install(FILES users DESTINATION share/${PROJECT_NAME})

# Install all files from the files directory, excluding README.md
install(DIRECTORY files/ 
        DESTINATION share/${PROJECT_NAME}/files
        PATTERN "README.md" EXCLUDE)
