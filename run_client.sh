#!/bin/bash

echo "======================================="
echo "             Starting Client"
echo "======================================="
cd build/client
./file_client

if [ $? -ne 0 ]; then
    echo "Error: Failed to start the client."
    exit 1
fi

cd ..
