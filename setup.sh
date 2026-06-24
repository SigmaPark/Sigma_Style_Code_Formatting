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

# === run sak on app/ ===
echo
echo "=== sak app ==="
./build/sak app
