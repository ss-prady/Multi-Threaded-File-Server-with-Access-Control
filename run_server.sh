#!/bin/bash

clear
echo "=============================================="
echo "        FileServer Setup & Launch Script      "
echo "=============================================="

echo "[*] Creating build directory..."
mkdir -p build
cd build

echo "[*] Running CMake..."
cmake .. || { echo "[-] CMake configuration failed!"; exit 1; }

echo "[*] Building project..."
make || { echo "[-] Build failed!"; exit 1; }

echo "----------------------------------------------"
echo "[*] Launching File Server..."
cd server 
./file_server