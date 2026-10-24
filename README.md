# TACCo — Totally Another C Compiler

**TACCo** is an end-to-end compiler for a C-style language, written in C++17 and targeting 64-bit RISC-V (RV64). The compiler translates source code written in TACCo's language dialect through a multi-stage pipeline down to executable RISC-V assembly that can be assembled with GNU GCC and emulated on QEMU.

---

## 1. Compiler Pipeline Architecture

```
                    TACCo Source (.tc)
                            |
                            v
                     +--------------+
                     |  Flex Lexer  |   (Lexical Analysis & Tokenization)
                     +-------+------+
                             |
                           Tokens
                             |
                             v
                     +--------------+
                     | Bison Parser |   (LALR(1) Syntax Analysis)
                     +-------+------+
                             |
                            AST
                             |
                             v
                     +--------------+
                     | Symbol Table |   (Lexical Scopes & Name Resolution)
                     +-------+------+
                             |
                             v
                     +--------------+
                     | Type Checker |   (Semantic Analysis & Validation)
                     +-------+------+
                             |
                         Typed AST
                             |
                             v
                     +--------------+
                     | 3AC Builder  |   (Three-Address Code Generation)
                     +-------+------+
                             |
                          3AC IR
                             |
                             v
                     +--------------+
                     |  CFG Builder |   (Basic Blocks & Leader Analysis)
                     +-------+------+
                             |
                             v
                     +--------------+
                     |  Optimizer   |   (Constant Fold, Prop, DCE, CSE)
                     +-------+------+
                             |
                      Optimized 3AC
                             |
                             v
                     +--------------+
                     |  Reg Alloc   |   (Linear-Scan Register Allocation)
                     +-------+------+
                             |
                             v
                     +--------------+
                     | RISC-V Gen   |   (RV64 Assembly Emission)
                     +-------+------+
                             |
                      RISC-V Assembly (.s)
                             |
                             v
                     +--------------+
                     | GNU Toolchain|   (riscv64-linux-gnu-gcc)
                     +-------+------+
                             |
                        Executable
                             |
                             v
                     +--------------+
                     |  QEMU RV64   |   (qemu-riscv64 Emulation)
                     +--------------+
```

---

## 2. Implemented Features

* **Lexical Analysis (Flex)**:
  * Full tokenization of keywords (`int`, `char`, `void`, `if`, `else`, `while`, `for`, `return`, `class`).
  * Decimal and hexadecimal integer literals, character literals with escape sequences (`\n`, `\t`, `\r`, `\0`, `\\`, `\'`), string literals, and identifiers.
  * Single-line (`//`) and multi-line (`/* ... */`) comments.
  * Accurate source location tracking (`filename:line:column`) for diagnostic reporting.

* **Syntax Analysis (Bison)**:
  * LALR(1) grammar for declarations, expressions, control flow, functions, arrays, and classes.
  * Explicit operator precedence and associativity resolution.
  * Detailed syntax error messages with source coordinates.

* **Abstract Syntax Tree (AST)**:
  * Clean class hierarchy (`ASTNode`, `ExpressionNode`, `StatementNode`, `DeclarationNode`, `Type`, `ProgramNode`).
  * Double-dispatch Visitor Pattern (`ASTVisitor`) used by dumping, symbol collection, type checking, and IR generation.
  * Formatted hierarchical AST dumping (`--dump-ast`).

* **Symbol Table & Scoping**:
  * Hierarchical lexical scopes (Global, Function, Block, Class).
  * Variable declaration, shadowing, and resolution.
  * Class member declarations, field offsets, and total size computation.

* **Semantic Analysis & Type Checking**:
  * Type compatibility validation for primitive types (`int`, `char`, `void`), pointers (`T*`), arrays (`T[N]`), and classes.
  * Array-to-pointer decay.
  * Lvalue verification for assignments and address-of operations.
  * Return type validation and function argument count/type matching.
  * Member access (`pt.x`) and array subscript type checks.

* **Three-Address Code (3AC) IR**:
  * Linear quadruple/triple intermediate representation.
  * Virtual temporaries (`t0`, `t1`, ...) and explicit labels (`L0`, `L1`, ...).
  * Explicit memory instructions (`ADDR_OF`, `LOAD_PTR`, `STORE_PTR`, `LOAD_ARR`, `STORE_ARR`, `LOAD_FIELD`, `STORE_FIELD`).

* **Control Flow Graph (CFG) & Basic Blocks**:
  * Leader identification algorithm for basic block partitioning.
  * Predecessor and successor edge tracking.
  * Graphviz DOT export (`--dump-cfg`).

* **IR Optimization Passes**:
  * **Constant Folding**: Evaluates compile-time constant arithmetic, comparisons, and unary operations.
  * **Algebraic Simplification**: Simplifies identities (`x + 0 -> x`, `x * 1 -> x`, `x * 0 -> 0`, `x - x -> 0`).
  * **Constant Propagation**: Propagates known constants into downstream uses within and across basic blocks.
  * **Dead Code Elimination (DCE)**: Eliminates dead computations and unused virtual temporary definitions.
  * **Common Subexpression Elimination (CSE)**: Identifies redundant computations and reuses previously computed values.
  * **Peephole Optimization**: Local branch simplification and redundant instruction elimination.

* **RISC-V Code Generation**:
  * Target: RV64GC / RV64I (64-bit RISC-V).
  * Linear-scan register allocation (`LinearScanAllocator`) managing temporary registers `t2`–`t6` with stack spilling.
  * Standard RISC-V calling conventions (`a0`–`a7` for parameters and return values).
  * Stack frame management with 16-byte stack alignment, return address (`ra`) preservation, and frame pointer (`s0`) setup.
  * Read-only data section (`.rodata`) for string literals.

---

## 3. Project Structure

```
TACCo/
├── docs/                      # Architectural and technical documentation
│   ├── architecture.md        # Compiler architecture and phase details
│   ├── ir.md                  # Three-Address Code instruction set specification
│   ├── language-spec.md       # TACCo source language grammar and types
│   ├── optimization.md        # Description of optimization algorithms
│   └── riscv-backend.md       # RV64 ABI and stack layout specification
├── examples/                  # Test programs in TACCo language (.tc)
│   ├── arrays.tc              # Array manipulation and indexing
│   ├── basic.tc               # Minimal function returning constant
│   ├── classes.tc             # Class definitions and field access
│   ├── control.tc             # While loops, for loops, and if-else
│   ├── expressions.tc         # Complex arithmetic, relational, and logical expressions
│   ├── functions.tc           # Recursive function calls (factorial)
│   ├── optimization.tc        # Demonstration of constant folding and DCE
│   └── pointers.tc            # Pointer address-of and dereference
├── src/                       # Compiler implementation source code
│   ├── ast/                   # AST nodes, type representation, and AST dumper
│   │   ├── ast.hpp, ast.cpp, ast_dump.cpp
│   │   ├── declarations.hpp, expressions.hpp, statements.hpp
│   │   └── types.hpp, types.cpp
│   ├── cfg/                   # Control Flow Graph and basic block analysis
│   │   ├── cfg.hpp, cfg.cpp
│   ├── codegen/               # RV64 code emission and linear-scan register allocation
│   │   ├── regalloc.hpp, regalloc.cpp
│   │   └── riscv_codegen.hpp, riscv_codegen.cpp
│   ├── common/                # Shared utilities
│   │   └── location.hpp       # Source location tracking (file:line:col)
│   ├── ir/                    # Three-Address Code structures and IR builder
│   │   ├── ir.hpp, ir.cpp
│   │   └── ir_builder.hpp, ir_builder.cpp
│   ├── lexer/                 # Flex lexer specification and driver
│   │   ├── lexer.l
│   │   ├── lexer_driver.hpp, lexer_driver.cpp
│   │   └── token.hpp, token.cpp
│   ├── optimizer/             # Optimization passes
│   │   ├── optimizer.hpp, optimizer.cpp
│   ├── parser/                # Bison grammar and parser driver
│   │   ├── parser.y
│   │   └── parser_driver.hpp, parser_driver.cpp
│   ├── semantic/              # Symbol table, scopes, and type checking
│   │   ├── scope.hpp, scope.cpp
│   │   ├── symbol.hpp, symbol.cpp
│   │   ├── symbol_table.hpp, symbol_table.cpp
│   │   └── type_checker.hpp, type_checker.cpp
│   └── main.cpp               # CLI entry point and pipeline driver
├── tests/                     # Unit and integration test suites
│   ├── cfg/test_cfg.cpp
│   ├── codegen/test_codegen.cpp
│   ├── e2e/test_e2e.cpp
│   ├── ir/test_ir.cpp
│   ├── lexer/test_lexer.cpp
│   ├── optimizer/test_optimizer.cpp
│   ├── parser/test_parser.cpp
│   └── semantic/test_semantic.cpp, test_symbol_table.cpp
├── Makefile                   # Build automation configuration
├── README.md                  # Main documentation
└── test_qemu.sh               # Execution script for RV64 binaries via QEMU
```

---

## 4. Prerequisites & Build Instructions

### Prerequisites
- **Linux** or **Windows Subsystem for Linux (WSL2)**:
  - GCC / G++ (C++17 support, GCC >= 9.0)
  - Flex (>= 2.6.4)
  - Bison (>= 3.8)
  - GNU Make
- **RISC-V Execution Tools (Optional for running RV64 binaries)**:
  - `gcc-riscv64-linux-gnu` (cross-compiler and static runtime)
  - `qemu-user` (`qemu-riscv64` user-mode emulator)

On Ubuntu/Debian (or WSL Ubuntu):
```bash
sudo apt update
sudo apt install build-essential flex bison gcc-riscv64-linux-gnu qemu-user
```

### Building TACCo
To compile the compiler executable (`bin/tacco`):
```bash
make
```

To clean all build artifacts and generated files:
```bash
make clean
```

---

## 5. Running Tests

### Unit and Integration Test Suite
TACCo contains 59 unit and end-to-end integration tests covering all compiler phases:
```bash
make test
```

Individual test modules:
```bash
make test-lexer       # Lexical analyzer tests
make test-parser      # Grammar and AST parsing tests
make test-symtab      # Scopes and symbol table tests
make test-semantic    # Type checker tests (positive & negative cases)
make test-ir          # 3AC Intermediate Representation tests
make test-cfg         # Basic block & CFG analysis tests
make test-opt         # Constant folding, propagation, CSE, DCE tests
make test-codegen     # Register allocation & RV64 emission tests
make test-e2e         # End-to-end pipeline compilation tests
```

### Full RV64 Execution on QEMU
To compile all 8 example programs to RISC-V assembly, assemble and link them with the RISC-V GNU cross-toolchain, and run them under QEMU:
```bash
make test-qemu
# Or directly:
./test_qemu.sh
```

Expected exit codes:
* `arrays.tc`       -> 30
* `basic.tc`        -> 42
* `classes.tc`      -> 50
* `control.tc`      -> 35
* `expressions.tc`  -> 48
* `functions.tc`    -> 120
* `optimization.tc` -> 60
* `pointers.tc`     -> 200

---

## 6. Command Line Interface (CLI)

```
Usage: ./bin/tacco [options] <source-file>

Options:
  -t, --dump-tokens    Tokenize source file and print token stream
  -a, --dump-ast       Parse source file and print Abstract Syntax Tree (AST)
  -s, --dump-symbols   Build symbol table and print scopes/symbols
  -c, --check          Perform syntax and semantic type-checking only
  -i, --dump-ir        Generate and print Three Address Code (3AC) IR
      --dump-cfg       Generate and print Control Flow Graph (DOT format)
  -O, --optimize       Enable compiler optimization passes
  -S, --emit-asm       Emit 64-bit RISC-V (RV64) assembly code
  -o <file>            Specify output file name
  -v, --version        Display compiler version
  -h, --help           Display this help message
```

### Example Invocations

1. **Emit RISC-V Assembly**:
   ```bash
   ./bin/tacco -S examples/functions.tc -o functions.s
   ```

2. **Dump Three-Address Code (3AC) IR**:
   ```bash
   ./bin/tacco --dump-ir examples/classes.tc
   ```

3. **Dump Optimized 3AC with Constant Folding & DCE**:
   ```bash
   ./bin/tacco -O --dump-ir examples/optimization.tc
   ```

4. **Export Control Flow Graph in Graphviz DOT Format**:
   ```bash
   ./bin/tacco --dump-cfg examples/control.tc > control.dot
   dot -Tpng control.dot -o control.png
   ```

5. **Token Stream & AST Inspection**:
   ```bash
   ./bin/tacco -t examples/basic.tc
   ./bin/tacco -a examples/basic.tc
   ```

---

## 7. Technical Notes & Viva Highlights

1. **LALR(1) Parsing & Grammar Ambiguity Resolution**:
   In C-style grammars with user-defined types (e.g. `class Point { ... };`), the sequence `Identifier * Identifier;` presents a lookahead ambiguity:
   * Is it a pointer variable declaration (`Point *pt;`)?
   * Or is it an expression statement (`a * b;`)?
   In an LALR(1) parser without type-feedback from the lexer ("the lexer hack"), this manifests as a reduce/reduce conflict on `*`. Bison resolves this by selecting the rule listed first in the grammar file (`type_specifier`), which correctly treats `ID * ID;` as a declaration.

2. **Memory Layout and Calling Conventions**:
   * **Classes**: Contiguous memory allocated on the stack frame; field offsets are calculated based on field sizes and 8-byte alignment constraints.
   * **Calling Convention**: Conforms to standard 64-bit RISC-V calling conventions. First 8 arguments and return values are passed via `a0`–`a7`.
   * **Stack Alignment**: Stack pointer (`sp`) is strictly kept 16-byte aligned throughout function execution. Frame pointer (`s0`) anchors local variable and spilled register accesses.

3. **Intermediate Representation**:
   Linear 3AC was chosen over a tree or SSA IR because it cleanly bridges the semantic gap between the high-level typed AST and the low-level RISC-V instruction set, enabling straightforward basic block formation and register allocation.
