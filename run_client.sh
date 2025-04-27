clear
echo "=============================================="
echo "        FileClient Setup & Launch Script      "
echo "=============================================="

echo "[*] Creating build directory..."
mkdir -p build
cd build

echo "[*] Running CMake..."
cmake .. || { echo "[-] CMake configuration failed!"; exit 1; }

echo "[*] Building project..."
make || { echo "[-] Build failed!"; exit 1; }

echo "----------------------------------------------"
echo "[*] Launching File Client..."
cd client 
./file_client