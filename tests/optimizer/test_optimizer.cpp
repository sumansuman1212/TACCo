#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "../../src/parser/parser_driver.hpp"
#include "../../src/semantic/type_checker.hpp"
#include "../../src/ir/ir_builder.hpp"
#include "../../src/optimizer/optimizer.hpp"

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

static std::unique_ptr<IRProgram> build_ir_from_source(const std::string& src) {
    ParserDriver parser;
    if (!parser.parse_string(src, "test.tc") || parser.has_errors()) return nullptr;
    TypeChecker checker;
    if (!checker.check(*parser.get_ast()) || checker.has_errors()) return nullptr;
    IRBuilder builder;
    return builder.build(*parser.get_ast());
}

static bool test_constant_folding() {
    std::string src = "int main() { int x = (10 + 20) * 2; return x; }";
    auto ir = build_ir_from_source(src);
    TEST_ASSERT(ir != nullptr, "IR generated");
    
    Optimizer opt;
    opt.optimize(*ir);

    // Look for folded constant 60
    bool found_60 = false;
    for (const auto& inst : ir->functions[0].instructions) {
        if (inst.src1.is_const() && inst.src1.val == 60) {
            found_60 = true;
            break;
        }
    }
    TEST_ASSERT(found_60, "Constant folded to 60");
    return true;
}

static bool test_algebraic_simplification() {
    std::string src = "int main() { int a = 42; int b = a * 1 + 0; return b; }";
    auto ir = build_ir_from_source(src);
    TEST_ASSERT(ir != nullptr, "IR generated");

    Optimizer opt;
    opt.optimize(*ir);

    // After algebraic simplification and propagation, no MUL with 1 should exist
    bool has_mul_one = false;
    for (const auto& inst : ir->functions[0].instructions) {
        if (inst.op == Opcode::MUL && (inst.src1.val == 1 || inst.src2.val == 1)) {
            has_mul_one = true;
        }
    }
    TEST_ASSERT(!has_mul_one, "Algebraic identity simplified");
    return true;
}

static bool test_dead_code_elimination() {
    std::string src = "int main() { int x = 10; int dead = 999 * 888; return x; }";
    auto ir = build_ir_from_source(src);
    TEST_ASSERT(ir != nullptr, "IR generated");

    Optimizer opt;
    opt.optimize(*ir);

    // Dead computation 999 * 888 should be eliminated
    bool found_dead_const = false;
    for (const auto& inst : ir->functions[0].instructions) {
        if (inst.src1.val == 887112 || inst.src2.val == 887112) {
            found_dead_const = true;
        }
    }
    TEST_ASSERT(!found_dead_const, "Dead computation eliminated");
    return true;
}

static bool test_common_subexpression_elimination() {
    std::string src = "int main() { int a = 10; int b = 20; int x = a + b; int y = a + b; return x + y; }";
    auto ir = build_ir_from_source(src);
    TEST_ASSERT(ir != nullptr, "IR generated");

    Optimizer opt;
    opt.optimize(*ir);

    // Count how many ADD instructions occur (should be simplified or CSE'd)
    int add_count = 0;
    for (const auto& inst : ir->functions[0].instructions) {
        if (inst.op == Opcode::ADD) add_count++;
    }
    // Originally had 3 ADDs, should be reduced
    TEST_ASSERT(add_count < 3, "Common subexpression eliminated");
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;

    std::cout << "========================================\n";
    std::cout << "TACCo Phase 8: Optimization Unit Tests\n";
    std::cout << "========================================\n";

    RUN_TEST(test_constant_folding);
    RUN_TEST(test_algebraic_simplification);
    RUN_TEST(test_dead_code_elimination);
    RUN_TEST(test_common_subexpression_elimination);

    std::cout << "========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";

    return (failed == 0) ? 0 : 1;
}
