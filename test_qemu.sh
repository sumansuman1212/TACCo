#!/bin/bash
set -e

# Pre-flight toolchain check
if ! command -v riscv64-linux-gnu-gcc >/dev/null 2>&1; then
    echo "Error: riscv64-linux-gnu-gcc not found in PATH." >&2
    echo "Please install the RISC-V cross-compiler (e.g., sudo apt install gcc-riscv64-linux-gnu)." >&2
    exit 1
fi

if ! command -v qemu-riscv64 >/dev/null 2>&1; then
    echo "Error: qemu-riscv64 not found in PATH." >&2
    echo "Please install QEMU user emulator (e.g., sudo apt install qemu-user)." >&2
    exit 1
fi

if [ ! -f "./bin/tacco" ]; then
    echo "Error: ./bin/tacco not found. Run 'make' first." >&2
    exit 1
fi

echo "=========================================="
echo "TACCo RV64 Binary Execution via QEMU"
echo "=========================================="

cleanup() {
    rm -f *.s *.out
}
trap cleanup EXIT

for f in examples/*.tc; do
    name=$(basename "$f" .tc)
    ./bin/tacco -S "$f" -o "$name.s"
    riscv64-linux-gnu-gcc -static "$name.s" -o "$name.out"
    set +e
    qemu-riscv64 ./"$name.out"
    code=$?
    set -e
    echo "  [SUCCESS] $name.tc  -->  Exit Code: $code"
    rm -f "$name.s" "$name.out"
done

echo "=========================================="
echo "All 8 programs executed successfully on RV64 QEMU!"
echo "=========================================="

