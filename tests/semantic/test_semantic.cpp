#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include "../../src/parser/parser_driver.hpp"
#include "../../src/semantic/type_checker.hpp"

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

static bool check_valid_source(const std::string& src, const std::string& name = "valid.tc") {
    ParserDriver parser;
    bool parse_ok = parser.parse_string(src, name);
    if (!parse_ok || parser.has_errors()) return false;
    TypeChecker checker;
    return checker.check(*parser.get_ast()) && !checker.has_errors();
}

static bool check_invalid_source(const std::string& src, const std::string& name = "invalid.tc") {
    ParserDriver parser;
    bool parse_ok = parser.parse_string(src, name);
    if (!parse_ok || parser.has_errors()) return true; // Rejected at parse or semantic
    TypeChecker checker;
    bool sem_ok = checker.check(*parser.get_ast());
    return !sem_ok || checker.has_errors();
}

static bool test_positive_programs() {
    std::string sample = 
        "class Point {\n"
        "    int x;\n"
        "    int y;\n"
        "};\n"
        "\n"
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n"
        "\n"
        "int main() {\n"
        "    int arr[4];\n"
        "    int x = 10;\n"
        "    int *p = &x;\n"
        "    arr[0] = *p;\n"
        "    arr[1] = add(20, 30);\n"
        "    Point pt;\n"
        "    pt.x = arr[0];\n"
        "    pt.y = arr[1];\n"
        "    if (pt.x < pt.y) {\n"
        "        return pt.y;\n"
        "    }\n"
        "    return pt.x;\n"
        "}\n";

    TEST_ASSERT(check_valid_source(sample, "sample.tc"), "Canonical Point program passes type-checking");
    return true;
}

static bool test_undeclared_variable_error() {
    std::string src = "int main() { int x = y + 10; return x; }";
    TEST_ASSERT(check_invalid_source(src), "Undeclared variable 'y' flagged as error");
    return true;
}

static bool test_assignment_type_mismatch() {
    std::string src = "int main() { int x = 10; int *p; p = x; return 0; }";
    TEST_ASSERT(check_invalid_source(src), "Assigning int to int* flagged as error");
    return true;
}

static bool test_address_of_rvalue_error() {
    std::string src = "int main() { int *p = &(10 + 20); return 0; }";
    TEST_ASSERT(check_invalid_source(src), "Address-of rvalue flagged as error");
    return true;
}

static bool test_dereference_non_pointer_error() {
    std::string src = "int main() { int x = 10; int y = *x; return y; }";
    TEST_ASSERT(check_invalid_source(src), "Dereferencing non-pointer flagged as error");
    return true;
}

static bool test_function_argument_count_mismatch() {
    std::string src = "int add(int a, int b) { return a + b; } int main() { return add(10); }";
    TEST_ASSERT(check_invalid_source(src), "Argument count mismatch flagged as error");
    return true;
}

static bool test_function_argument_type_mismatch() {
    std::string src = "class Point { int x; }; int foo(int a) { return a; } int main() { Point p; return foo(p); }";
    TEST_ASSERT(check_invalid_source(src), "Argument type mismatch flagged as error");
    return true;
}

static bool test_class_unknown_member_error() {
    std::string src = "class Point { int x; int y; }; int main() { Point p; p.z = 10; return 0; }";
    TEST_ASSERT(check_invalid_source(src), "Accessing non-existent member 'z' flagged as error");
    return true;
}

static bool test_member_access_on_non_class_error() {
    std::string src = "int main() { int a = 10; a.x = 20; return 0; }";
    TEST_ASSERT(check_invalid_source(src), "Member access on integer variable flagged as error");
    return true;
}

static bool test_array_subscript_type_error() {
    std::string src = "int main() { int arr[5]; int *p; arr[p] = 10; return 0; }";
    TEST_ASSERT(check_invalid_source(src), "Indexing array with pointer flagged as error");
    return true;
}

static bool test_return_type_mismatch() {
    std::string src1 = "void test() { return 42; }";
    TEST_ASSERT(check_invalid_source(src1), "Returning value in void function flagged as error");

    std::string src2 = "int test() { return; }";
    TEST_ASSERT(check_invalid_source(src2), "Returning empty in non-void function flagged as error");
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;

    std::cout << "========================================\n";
    std::cout << "TACCo Phase 5: Semantic & Type Checking Unit Tests\n";
    std::cout << "========================================\n";

    RUN_TEST(test_positive_programs);
    RUN_TEST(test_undeclared_variable_error);
    RUN_TEST(test_assignment_type_mismatch);
    RUN_TEST(test_address_of_rvalue_error);
    RUN_TEST(test_dereference_non_pointer_error);
    RUN_TEST(test_function_argument_count_mismatch);
    RUN_TEST(test_function_argument_type_mismatch);
    RUN_TEST(test_class_unknown_member_error);
    RUN_TEST(test_member_access_on_non_class_error);
    RUN_TEST(test_array_subscript_type_error);
    RUN_TEST(test_return_type_mismatch);

    std::cout << "========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";

    return (failed == 0) ? 0 : 1;
}
