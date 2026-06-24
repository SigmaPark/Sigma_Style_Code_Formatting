#!/bin/sh
set -e

# === configure / build ===
if [ ! -d build ]; then
	mkdir build
fi
cd build
cmake ..
cmake --build .
cd ..

# === self-check: run sak on app/ recursively ===
echo
echo "=== sak -r app ==="
./build/sak -r app
