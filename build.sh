#!/bin/bash

# Magic IDE Build Script
# One-command build for the entire project

set -e  # Exit on any error

echo "🚀 Building Magic IDE..."

# Create build directory if it doesn't exist
mkdir -p build

# Navigate to build directory
cd build

# Configure with CMake
echo "📋 Configuring with CMake..."
cmake ..

# Build with make
echo "🔨 Building with make..."
make

# Check if executable was created
if [ -f "magic-ide" ]; then
    echo "✅ Build successful!"
    echo "🎉 Magic IDE executable created: $(pwd)/magic-ide"
    echo "📏 Size: $(ls -lh magic-ide | awk '{print $5}')B"
else
    echo "❌ Build failed - executable not found"
    exit 1
fi

echo ""
echo "To run Magic IDE:"
echo "  cd build && ./magic-ide"
echo "  (Requires graphical desktop environment)"