# #!/bin/bash

# clear
# echo "=============================================="
# echo "        FileServer Setup & Launch Script      "
# echo "=============================================="

# echo "[*] Creating build directory..."
# mkdir -p build
# cd build

# echo "[*] Running CMake..."
# cmake .. || { echo "[-] CMake configuration failed!"; exit 1; }

# echo "[*] Building project..."
# make || { echo "[-] Build failed!"; exit 1; }

# echo "----------------------------------------------"
# echo "[*] Launching File Server..."
# cd server 
# ./file_server

#!/bin/bash

clear
echo "=============================================="
echo "        FileServer Setup & Launch Script      "
echo "=============================================="

echo "[*] Creating build directory..."
mkdir -p build
cd build

echo "[*] Running CMake..."
cmake ..       || { echo "[-] CMake configuration failed!"; exit 1; }

echo "[*] Building project..."
make           || { echo "[-] Build failed!"; exit 1; }

echo "----------------------------------------------"
echo "[*] Preparing user accounts..."
cd server

# Loop: ask whether to add a user; if yes, prompt for details and call user_manager.sh
while true; do
  read -p "Do you want to add a user? (y/n) " yn
  case "$yn" in
    [Yy]* )
      read -p "  Enter user_name: " file_name
      read -p "  Enter password: " password
      read -p "  Enter role: " role
      echo "[*] Adding user: user='$file_name', role='$role'"
      ./user_manager -a "$file_name" "$password" "$role" \
         || { echo "[-] Failed to add user!"; exit 1; }
      ;;
    [Nn]* )
      echo "[*] User setup complete."
      break
      ;;
    * )
      echo "Please answer y or n."
      ;;
  esac
done

echo "----------------------------------------------"
echo "[*] Launching File Server..."
./file_server
