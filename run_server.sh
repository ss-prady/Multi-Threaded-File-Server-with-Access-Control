#!/bin/bash

clear
echo "=============================================="
echo "        FileServer Setup & Launch Script      "
echo "=============================================="

# Function to check if a command exists
check_dependency() {
    if ! command -v $1 &> /dev/null; then
        echo "[-] Error: $1 is not installed. Please install it and try again."
        exit 1
    else
        echo "[✓] $1 found."
    fi
}

# Step 0: Check for required tools
echo "[*] Checking for dependencies..."
check_dependency cmake
check_dependency gcc
check_dependency make

# Step 1: Create and navigate to build directory
echo "[*] Creating build directory..."
mkdir -p build
cd build

# Step 2: Run CMake
echo "[*] Running CMake..."
cmake .. || { echo "[-] CMake configuration failed!"; exit 1; }

# Step 3: Build the server and client
echo "[*] Building project..."
make || { echo "[-] Build failed!"; exit 1; }

# Step 4: Run the server
echo "----------------------------------------------"
echo "[*] Launching File Server..."
cd server 
./file_server

# Step 5: Return to project root
cd ..