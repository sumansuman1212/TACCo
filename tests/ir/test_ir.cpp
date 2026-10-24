#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "../../src/parser/parser_driver.hpp"
#include "../../src/semantic/type_checker.hpp"
#include "../../src/ir/ir_builder.hpp"

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
    if (!parser.parse_string(src, "test.tc") || parser.has_errors()) {
        return nullptr;
    }
    TypeChecker checker;
    if (!checker.check(*parser.get_ast()) || checker.has_errors()) {
        return nullptr;
    }
    IRBuilder builder;
    return builder.build(*parser.get_ast());
}

static bool test_basic_ir_generation() {
    std::string src = "int main() { int x = 42; return x; }";
    auto ir = build_ir_from_source(src);
    TEST_ASSERT(ir != nullptr, "IR generated for basic program");
    TEST_ASSERT(ir->functions.size() == 1, "One function generated");
    TEST_ASSERT(ir->functions[0].name == "main", "Function is named main");
    TEST_ASSERT(!ir->functions[0].instructions.empty(), "Instructions generated");
    return true;
}

static bool test_arithmetic_and_expressions_ir() {
    std::string src = "int main() { int a = 10; int b = 20; int c = a + b * 2; return c; }";
    auto ir = build_ir_from_source(src);
    TEST_ASSERT(ir != nullptr, "IR generated for arithmetic expressions");
    const auto& insts = ir->functions[0].instructions;
    bool has_mul = false, has_add = false;
    for (const auto& inst : insts) {
        if (inst.op == Opcode::MUL) has_mul = true;
        if (inst.op == Opcode::ADD) has_add = true;
    }
    TEST_ASSERT(has_mul, "Contains MUL instruction");
    TEST_ASSERT(has_add, "Contains ADD instruction");
    return true;
}

static bool test_control_flow_ir() {
    std::string src = "int main() { int i = 0; while (i < 5) { i = i + 1; } return i; }";
    auto ir = build_ir_from_source(src);
    TEST_ASSERT(ir != nullptr, "IR generated for while loop");
    const auto& insts = ir->functions[0].instructions;
    bool has_if_false = false, has_goto = false, has_label = false;
    for (const auto& inst : insts) {
        if (inst.op == Opcode::IF_FALSE) has_if_false = true;
        if (inst.op == Opcode::GOTO) has_goto = true;
        if (inst.op == Opcode::LABEL) has_label = true;
    }
    TEST_ASSERT(has_if_false, "Contains conditional branch");
    TEST_ASSERT(has_goto, "Contains unconditional jump");
    TEST_ASSERT(has_label, "Contains loop labels");
    return true;
}

static bool test_functions_and_calls_ir() {
    std::string src = "int add(int a, int b) { return a + b; } int main() { return add(5, 10); }";
    auto ir = build_ir_from_source(src);
    TEST_ASSERT(ir != nullptr, "IR generated for functions");
    TEST_ASSERT(ir->functions.size() == 2, "Two functions generated");
    bool has_param = false, has_call = false;
    for (const auto& inst : ir->functions[1].instructions) {
        if (inst.op == Opcode::PARAM) has_param = true;
        if (inst.op == Opcode::CALL) has_call = true;
    }
    TEST_ASSERT(has_param, "Contains PARAM instructions");
    TEST_ASSERT(has_call, "Contains CALL instruction");
    return true;
}

static bool test_arrays_and_pointers_ir() {
    std::string src = "int main() { int arr[3]; arr[1] = 42; int *p = &arr[1]; return *p; }";
    auto ir = build_ir_from_source(src);
    TEST_ASSERT(ir != nullptr, "IR generated for arrays and pointers");
    bool has_store_arr = false, has_load_ptr = false;
    for (const auto& inst : ir->functions[0].instructions) {
        if (inst.op == Opcode::STORE_ARR) has_store_arr = true;
        if (inst.op == Opcode::LOAD_PTR) has_load_ptr = true;
    }
    TEST_ASSERT(has_store_arr, "Contains STORE_ARR instruction");
    TEST_ASSERT(has_load_ptr, "Contains LOAD_PTR instruction");
    return true;
}

static bool test_classes_ir() {
    std::string src = "class Point { int x; int y; }; int main() { Point pt; pt.x = 10; pt.y = 20; return pt.x + pt.y; }";
    auto ir = build_ir_from_source(src);
    TEST_ASSERT(ir != nullptr, "IR generated for class");
    TEST_ASSERT(ir->classes.find("Point") != ir->classes.end(), "Class Point registered in IR");
    TEST_ASSERT(ir->classes["Point"].total_size >= 16, "Class size computed");
    bool has_store_field = false, has_load_field = false;
    for (const auto& inst : ir->functions[0].instructions) {
        if (inst.op == Opcode::STORE_FIELD) has_store_field = true;
        if (inst.op == Opcode::LOAD_FIELD) has_load_field = true;
    }
    TEST_ASSERT(has_store_field, "Contains STORE_FIELD instruction");
    TEST_ASSERT(has_load_field, "Contains LOAD_FIELD instruction");
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;

    std::cout << "========================================\n";
    std::cout << "TACCo Phase 6: Three Address Code (3AC) Unit Tests\n";
    std::cout << "========================================\n";

    RUN_TEST(test_basic_ir_generation);
    RUN_TEST(test_arithmetic_and_expressions_ir);
    RUN_TEST(test_control_flow_ir);
    RUN_TEST(test_functions_and_calls_ir);
    RUN_TEST(test_arrays_and_pointers_ir);
    RUN_TEST(test_classes_ir);

    std::cout << "========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";

    return (failed == 0) ? 0 : 1;
}
