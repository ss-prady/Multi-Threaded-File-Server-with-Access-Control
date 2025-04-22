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

# Copy users.txt to build directory
configure_file(users.txt users.txt COPYONLY)

# Installation rules
install(TARGETS file_server user_manager DESTINATION bin)
install(FILES users.txt DESTINATION share/${PROJECT_NAME}) 