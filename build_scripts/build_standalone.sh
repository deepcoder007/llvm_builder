#!/bin/bash
# Build standalone distribution that bundles LLVM libraries
# Users of this package do NOT need LLVM installed

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_TYPE="${1:-Release}"
BUILD_DIR="$PROJECT_DIR/build_standalone"
INSTALL_DIR="$PROJECT_DIR/artifacts/standalone_${BUILD_TYPE}"

echo "=== Building Standalone Distribution ==="
echo "Build type: $BUILD_TYPE"
echo "Install dir: $INSTALL_DIR"

# Clean previous build
rm -rf "$BUILD_DIR" "$INSTALL_DIR"
mkdir -p "$BUILD_DIR" "$INSTALL_DIR"

cd "$BUILD_DIR"

# Configure with bundling enabled
cmake -GNinja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DBUILD_PYTHON_BINDINGS=ON \
    -DLLVM_BUILDER_BUNDLE_LLVM=ON \
    "$PROJECT_DIR"

# Build
ninja

# Install
ninja install

# Bundle LLVM and its dependencies
echo "=== Bundling LLVM libraries ==="

LIBS_DIR="$INSTALL_DIR/lib/.libs"
mkdir -p "$LIBS_DIR"

# Find the LLVM shared library being used
LLVM_LIB=$(ldd "$INSTALL_DIR"/lib/python*/site-packages/*.so 2>/dev/null | grep -E 'libLLVM' | awk '{print $3}' | head -1)

if [ -n "$LLVM_LIB" ] && [ -f "$LLVM_LIB" ]; then
    echo "Found LLVM library: $LLVM_LIB"
    cp "$LLVM_LIB" "$LIBS_DIR/"

    # Also copy any LLVM-specific dependencies (like libstdc++ if needed)
    ldd "$LLVM_LIB" 2>/dev/null | while read -r line; do
        lib_path=$(echo "$line" | awk '{print $3}')
        lib_name=$(echo "$line" | awk '{print $1}')
        # Copy LLVM-related and potentially missing runtime libs
        if [[ "$lib_name" == libLLVM* ]] || [[ "$lib_name" == libclang* ]]; then
            if [ -f "$lib_path" ]; then
                echo "  Bundling: $lib_name"
                cp "$lib_path" "$LIBS_DIR/"
            fi
        fi
    done
else
    echo "Warning: Could not find LLVM shared library"
fi

# Fix RPATH on the Python extension to find bundled libs
for so_file in "$INSTALL_DIR"/lib/python*/site-packages/*.so; do
    if [ -f "$so_file" ]; then
        echo "Setting RPATH on: $so_file"
        patchelf --set-rpath '$ORIGIN/.libs' "$so_file" 2>/dev/null || \
            chrpath -r '$ORIGIN/.libs' "$so_file" 2>/dev/null || \
            echo "  Warning: Could not set RPATH (install patchelf or chrpath)"
    fi
done

# Copy bundled libs to site-packages as well
SITE_PACKAGES=$(dirname "$so_file")
if [ -d "$SITE_PACKAGES" ]; then
    cp -r "$LIBS_DIR" "$SITE_PACKAGES/.libs" 2>/dev/null || true
fi

echo ""
echo "=== Standalone build complete ==="
echo "Output: $INSTALL_DIR"
echo ""
echo "Contents:"
find "$INSTALL_DIR" -type f | head -20
echo ""
echo "To use the C++ library, add to your CMakeLists.txt:"
echo "  list(APPEND CMAKE_PREFIX_PATH \"$INSTALL_DIR\")"
echo "  find_package(llvm_builder REQUIRED)"
echo ""
echo "To use the Python module:"
echo "  export PYTHONPATH=\"$SITE_PACKAGES:\$PYTHONPATH\""
echo "  python -c 'import llvm_builder_py'"
