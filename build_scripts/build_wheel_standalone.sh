#!/bin/bash
# Build a Python wheel with all LLVM dependencies bundled
# The resulting wheel can be installed without requiring LLVM on the target system

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/artifacts/wheels"

echo "=== Building Standalone Python Wheel ==="

# Check for required tools
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo "Error: $1 is required but not installed"
        echo "Install with: $2"
        exit 1
    fi
}

check_tool pip "python -m pip install pip"
check_tool auditwheel "pip install auditwheel"

# Clean output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

cd "$PROJECT_DIR"

# Build the wheel
echo "Building wheel..."
pip wheel . --no-deps -w "$OUTPUT_DIR/raw"

# Find the built wheel
WHEEL=$(find "$OUTPUT_DIR/raw" -name "*.whl" | head -1)
if [ -z "$WHEEL" ]; then
    echo "Error: No wheel file found"
    exit 1
fi

echo "Built wheel: $WHEEL"

# Repair the wheel to bundle shared libraries
echo ""
echo "Repairing wheel with auditwheel (bundling dependencies)..."

# Use manylinux_2_28 as it's a reasonable modern baseline
# This bundles libLLVM and other dependencies into the wheel
auditwheel repair "$WHEEL" \
    --plat manylinux_2_28_x86_64 \
    -w "$OUTPUT_DIR" \
    2>&1 | tee "$OUTPUT_DIR/auditwheel.log"

# Show results
echo ""
echo "=== Build Complete ==="
echo ""
echo "Standalone wheels:"
ls -lh "$OUTPUT_DIR"/*.whl 2>/dev/null || echo "(check $OUTPUT_DIR)"

echo ""
echo "To install on any Linux system (no LLVM required):"
echo "  pip install $OUTPUT_DIR/llvm_builder_py-*.whl"

# Cleanup raw wheel
rm -rf "$OUTPUT_DIR/raw"
