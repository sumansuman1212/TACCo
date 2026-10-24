#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include "../../src/parser/parser_driver.hpp"
#include "../../src/semantic/type_checker.hpp"
#include "../../src/ir/ir_builder.hpp"
#include "../../src/cfg/cfg.hpp"
#include "../../src/optimizer/optimizer.hpp"
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

static bool compile_example_file(const std::string& path, bool optimize) {
    ParserDriver parser;
    bool parse_ok = parser.parse_file(path);
    if (!parse_ok || parser.has_errors() || !parser.get_ast()) return false;

    TypeChecker checker;
    bool sem_ok = checker.check(*parser.get_ast());
    if (!sem_ok || checker.has_errors()) return false;

    IRBuilder ir_builder;
    auto ir = ir_builder.build(*parser.get_ast());
    if (!ir) return false;

    // Test CFG construction
    auto pcfg = ProgramCFG::build_from_program(*ir);
    if (!pcfg) return false;

    if (optimize) {
        Optimizer opt;
        opt.optimize(*ir);
    }

    RISCVCodeGenerator codegen;
    std::string asm_code = codegen.generate(*ir);
    if (asm_code.empty()) return false;

    return (asm_code.find(".globl  main") != std::string::npos &&
            asm_code.find("ret") != std::string::npos);
}

static bool test_e2e_basic() {
    TEST_ASSERT(compile_example_file("examples/basic.tc", false), "Compile basic.tc");
    return true;
}

static bool test_e2e_expressions() {
    TEST_ASSERT(compile_example_file("examples/expressions.tc", false), "Compile expressions.tc");
    return true;
}

static bool test_e2e_control() {
    TEST_ASSERT(compile_example_file("examples/control.tc", false), "Compile control.tc");
    return true;
}

static bool test_e2e_functions() {
    TEST_ASSERT(compile_example_file("examples/functions.tc", false), "Compile functions.tc");
    return true;
}

static bool test_e2e_arrays() {
    TEST_ASSERT(compile_example_file("examples/arrays.tc", false), "Compile arrays.tc");
    return true;
}

static bool test_e2e_pointers() {
    TEST_ASSERT(compile_example_file("examples/pointers.tc", false), "Compile pointers.tc");
    return true;
}

static bool test_e2e_classes() {
    TEST_ASSERT(compile_example_file("examples/classes.tc", false), "Compile classes.tc");
    return true;
}

static bool test_e2e_optimization() {
    TEST_ASSERT(compile_example_file("examples/optimization.tc", true), "Compile optimization.tc with -O");
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;

    std::cout << "========================================\n";
    std::cout << "TACCo Phase 13: End-to-End Pipeline Unit Tests\n";
    std::cout << "========================================\n";

    RUN_TEST(test_e2e_basic);
    RUN_TEST(test_e2e_expressions);
    RUN_TEST(test_e2e_control);
    RUN_TEST(test_e2e_functions);
    RUN_TEST(test_e2e_arrays);
    RUN_TEST(test_e2e_pointers);
    RUN_TEST(test_e2e_classes);
    RUN_TEST(test_e2e_optimization);

    std::cout << "========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";

    return (failed == 0) ? 0 : 1;
}
