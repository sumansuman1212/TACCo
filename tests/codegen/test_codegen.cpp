#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "../../src/parser/parser_driver.hpp"
#include "../../src/semantic/type_checker.hpp"
#include "../../src/ir/ir_builder.hpp"
#include "../../src/codegen/regalloc.hpp"
#include "../../src/codegen/riscv_codegen.hpp"

#define TEST_ASSERT(cond, msg)                                                \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::cerr << "[FAIL] " << msg << " (" << __FILE__ << ":"         \
                      << __LINE__ << ")\n";                                   \
            return false;                                                     \
        }                                                                     \
    } while (0)

#define RUN_TEST(fn)                                                          \
    do {                                                                      \
        std::cout << "Running " #fn "... ";                                   \
        if (fn()) {                                                           \
            std::cout << "PASSED\n";                                          \
            passed++;                                                         \
        } else {                                                              \
            failed++;                                                         \
        }                                                                     \
    } while (0)

using namespace tacco;

static std::string compile_to_asm(const std::string& src) {
    ParserDriver parser;
    if (!parser.parse_string(src, "test.tc") || parser.has_errors()) return "";
    TypeChecker checker;
    if (!checker.check(*parser.get_ast()) || checker.has_errors()) return "";
    IRBuilder builder;
    auto ir = builder.build(*parser.get_ast());
    if (!ir) return "";
    RISCVCodeGenerator codegen;
    return codegen.generate(*ir);
}

static bool test_linear_scan_allocator() {
    std::string src = "int main() { int a = 1; int b = 2; int c = a + b; return c; }";
    ParserDriver parser;
    parser.parse_string(src, "test.tc");
    TypeChecker checker;
    checker.check(*parser.get_ast());
    IRBuilder builder;
    auto ir = builder.build(*parser.get_ast());
    
    LinearScanAllocator regalloc;
    regalloc.allocate(ir->functions[0], 16);
    TEST_ASSERT(!regalloc.get_allocation().empty(), "Registers allocated");
    return true;
}

static bool test_basic_riscv_emission() {
    std::string src = "int main() { return 42; }";
    std::string asm_code = compile_to_asm(src);
    TEST_ASSERT(!asm_code.empty(), "Assembly generated");
    TEST_ASSERT(asm_code.find(".globl  main") != std::string::npos, "Has .globl main");
    TEST_ASSERT(asm_code.find("li a0, 42") != std::string::npos, "Loads 42 into a0");
    TEST_ASSERT(asm_code.find("ret") != std::string::npos, "Has ret instruction");
    return true;
}

static bool test_function_call_riscv() {
    std::string src = "int add(int a, int b) { return a + b; } int main() { return add(10, 20); }";
    std::string asm_code = compile_to_asm(src);
    TEST_ASSERT(!asm_code.empty(), "Assembly generated");
    TEST_ASSERT(asm_code.find(".globl  add") != std::string::npos, "Has add function");
    TEST_ASSERT(asm_code.find("call add") != std::string::npos, "Has call add instruction");
    return true;
}

static bool test_memory_and_classes_riscv() {
    std::string src = "class Point { int x; int y; }; int main() { Point p; p.x = 10; return p.x; }";
    std::string asm_code = compile_to_asm(src);
    TEST_ASSERT(!asm_code.empty(), "Assembly generated for classes");
    TEST_ASSERT(asm_code.find("sd") != std::string::npos, "Has store doubleword instruction");
    TEST_ASSERT(asm_code.find("ld") != std::string::npos, "Has load doubleword instruction");
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;

    std::cout << "========================================\n";
    std::cout << "TACCo Phase 9 & 10: RISC-V Codegen Unit Tests\n";
    std::cout << "========================================\n";

    RUN_TEST(test_linear_scan_allocator);
    RUN_TEST(test_basic_riscv_emission);
    RUN_TEST(test_function_call_riscv);
    RUN_TEST(test_memory_and_classes_riscv);

    std::cout << "========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";

    return (failed == 0) ? 0 : 1;
}
