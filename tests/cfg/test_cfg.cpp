#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "../../src/parser/parser_driver.hpp"
#include "../../src/semantic/type_checker.hpp"
#include "../../src/ir/ir_builder.hpp"
#include "../../src/cfg/cfg.hpp"

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

static std::unique_ptr<ControlFlowGraph> build_cfg_from_source(const std::string& src) {
    ParserDriver parser;
    if (!parser.parse_string(src, "test.tc") || parser.has_errors()) return nullptr;
    TypeChecker checker;
    if (!checker.check(*parser.get_ast()) || checker.has_errors()) return nullptr;
    IRBuilder builder;
    auto ir = builder.build(*parser.get_ast());
    if (!ir || ir->functions.empty()) return nullptr;
    return ControlFlowGraph::build_from_function(ir->functions[0]);
}

static bool test_linear_cfg() {
    std::string src = "int main() { int a = 1; int b = 2; int c = a + b; return c; }";
    auto cfg = build_cfg_from_source(src);
    TEST_ASSERT(cfg != nullptr, "CFG constructed");
    TEST_ASSERT(!cfg->blocks.empty(), "Blocks created");
    TEST_ASSERT(cfg->entry_block != nullptr, "Entry block exists");
    return true;
}

static bool test_branching_cfg() {
    std::string src = "int main() { int x = 10; if (x > 5) { x = x + 1; } else { x = x - 1; } return x; }";
    auto cfg = build_cfg_from_source(src);
    TEST_ASSERT(cfg != nullptr, "CFG constructed for branching");
    TEST_ASSERT(cfg->blocks.size() >= 3, "At least 3 blocks for if-else");
    // Entry block should have 2 successors (then and else)
    bool has_multi_succ = false;
    for (const auto& bb : cfg->blocks) {
        if (bb->successors.size() >= 2) {
            has_multi_succ = true;
            break;
        }
    }
    TEST_ASSERT(has_multi_succ, "Contains block with multiple successors");
    return true;
}

static bool test_loop_cfg() {
    std::string src = "int main() { int i = 0; int sum = 0; while (i < 10) { sum = sum + i; i = i + 1; } return sum; }";
    auto cfg = build_cfg_from_source(src);
    TEST_ASSERT(cfg != nullptr, "CFG constructed for loop");
    // Look for a back-edge (successor has smaller or equal block id)
    bool has_back_edge = false;
    for (const auto& bb : cfg->blocks) {
        for (const auto* succ : bb->successors) {
            if (succ->id <= bb->id) {
                has_back_edge = true;
                break;
            }
        }
    }
    TEST_ASSERT(has_back_edge, "Contains back-edge for loop");
    return true;
}

static bool test_dot_export() {
    std::string src = "int main() { int x = 5; if (x == 5) return 1; return 0; }";
    auto cfg = build_cfg_from_source(src);
    TEST_ASSERT(cfg != nullptr, "CFG constructed");
    std::string dot = cfg->to_dot();
    TEST_ASSERT(!dot.empty(), "DOT string generated");
    TEST_ASSERT(dot.find("digraph CFG_main") != std::string::npos, "DOT contains graph header");
    TEST_ASSERT(dot.find("node_0") != std::string::npos, "DOT contains nodes");
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;

    std::cout << "========================================\n";
    std::cout << "TACCo Phase 7: Basic Blocks & CFG Unit Tests\n";
    std::cout << "========================================\n";

    RUN_TEST(test_linear_cfg);
    RUN_TEST(test_branching_cfg);
    RUN_TEST(test_loop_cfg);
    RUN_TEST(test_dot_export);

    std::cout << "========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";

    return (failed == 0) ? 0 : 1;
}
