# llvm_builder

A C++ library to generate low latency binary code at runtime based on user input.
providing high-level abstractions for building, compiling, and execution with JIT support.

## Features

- **Type System** — Primitives (bool, int8–int64, uint8–uint64, float32, float64), pointers, arrays, vectors, structs, and function types
- **IR Construction** — Arithmetic, logical, memory, and cast operations through a fluent API
- **Control Flow** — Functions, if-else conditionals (with nesting), and hierarchical code sections
- **JIT Compilation** — OrcJIT-based execution with runtime object, array, and struct support
- **IR Analysis** — Module and function analysis, instruction enumeration, code block traversal
- **Python Bindings** — Optional nanobind-based Python module

## Requirements

- C++20 compiler (GCC or Clang)
- CMake 3.16+
- LLVM (Core, CodeGen, ExecutionEngine, IRReader, OrcJIT)
- Google Test (for unit tests)
- nanobind 1.3.2+ and Python 3.8+ (optional, for Python bindings)

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To build with Python bindings:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON_BINDINGS=ON
cmake --build build
```

## Quick Example

```cpp
#include <llvm_builder/module.h>
#include <llvm_builder/type.h>
#include <llvm_builder/value.h>
#include <llvm_builder/function.h>
#include <llvm_builder/jit.h>

// Create a cursor (compilation context)
Cursor cursor("example");
Module mod = cursor.main_module();

// Define a function that adds two i32 values
TypeInfo i32 = TypeInfo::mk_int32();
// ... build IR using ValueInfo operations ...

// JIT compile and execute
JustInTimeRunner jit;
jit.add_module(cursor);
auto* fn = jit.get_fn<int(int, int)>("add");
int result = fn(3, 4); // 7
```

## Running Tests

```bash
cmake --build build --target test
```

## Project Structure

```
TODO

## License

MIT
