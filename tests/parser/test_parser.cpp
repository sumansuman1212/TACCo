#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include "../../src/parser/parser_driver.hpp"
#include "../../src/ast/ast.hpp"
#include "../../src/ast/expressions.hpp"
#include "../../src/ast/statements.hpp"
#include "../../src/ast/declarations.hpp"

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

static bool test_parse_empty() {
    ParserDriver parser;
    bool ok = parser.parse_string("");
    TEST_ASSERT(ok, "Empty input parses successfully");
    TEST_ASSERT(parser.get_ast() != nullptr, "AST generated");
    TEST_ASSERT(parser.get_ast()->declarations.empty(), "No top-level declarations");
    return true;
}

static bool test_parse_variable_declarations() {
    std::string input = 
        "int a;\n"
        "int b = 42;\n"
        "char c = 'z';\n"
        "int arr[10];\n"
        "int *ptr = &b;\n";

    ParserDriver parser;
    bool ok = parser.parse_string(input);
    TEST_ASSERT(ok, "Variable declarations parsed successfully");
    auto& ast = parser.get_ast();
    TEST_ASSERT(ast != nullptr, "AST generated");
    TEST_ASSERT(ast->declarations.size() == 5, "5 declarations in AST");

    auto* var0 = dynamic_cast<VariableDeclarationNode*>(ast->declarations[0].get());
    TEST_ASSERT(var0 && var0->name == "a" && var0->type->is_int(), "Var 'a' type int");

    auto* var1 = dynamic_cast<VariableDeclarationNode*>(ast->declarations[1].get());
    TEST_ASSERT(var1 && var1->name == "b" && var1->initializer != nullptr, "Var 'b' with init");

    auto* var3 = dynamic_cast<VariableDeclarationNode*>(ast->declarations[3].get());
    TEST_ASSERT(var3 && var3->name == "arr" && var3->is_array && var3->array_size == 10, "Array var 'arr[10]'");

    auto* var4 = dynamic_cast<VariableDeclarationNode*>(ast->declarations[4].get());
    TEST_ASSERT(var4 && var4->name == "ptr" && var4->type->is_pointer(), "Pointer var 'ptr'");
    return true;
}

static bool test_parse_functions() {
    std::string input =
        "int compute(int x, int y);\n"
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n";

    ParserDriver parser;
    bool ok = parser.parse_string(input);
    TEST_ASSERT(ok, "Functions parsed successfully");
    auto& ast = parser.get_ast();
    TEST_ASSERT(ast->declarations.size() == 2, "2 declarations in AST");

    auto* decl0 = dynamic_cast<FunctionDeclarationNode*>(ast->declarations[0].get());
    TEST_ASSERT(decl0 && decl0->name == "compute" && decl0->params.size() == 2, "compute declaration");

    auto* def1 = dynamic_cast<FunctionDefinitionNode*>(ast->declarations[1].get());
    TEST_ASSERT(def1 && def1->name == "add" && def1->params.size() == 2, "add definition");
    TEST_ASSERT(def1->body != nullptr && def1->body->statements.size() == 1, "add body contains return");
    return true;
}

static bool test_parse_classes() {
    std::string input =
        "class Point {\n"
        "    int x;\n"
        "    int y;\n"
        "};\n"
        "class Buffer {\n"
        "    char data[128];\n"
        "    int size;\n"
        "};\n";

    ParserDriver parser;
    bool ok = parser.parse_string(input);
    TEST_ASSERT(ok, "Classes parsed successfully");
    auto& ast = parser.get_ast();
    TEST_ASSERT(ast->declarations.size() == 2, "2 classes in AST");

    auto* cls0 = dynamic_cast<ClassDeclarationNode*>(ast->declarations[0].get());
    TEST_ASSERT(cls0 && cls0->name == "Point" && cls0->fields.size() == 2, "Point class fields");
    TEST_ASSERT(cls0->fields[0]->name == "x" && cls0->fields[1]->name == "y", "Point x and y fields");

    auto* cls1 = dynamic_cast<ClassDeclarationNode*>(ast->declarations[1].get());
    TEST_ASSERT(cls1 && cls1->name == "Buffer" && cls1->fields.size() == 2, "Buffer class fields");
    TEST_ASSERT(cls1->fields[0]->is_array && cls1->fields[0]->array_size == 128, "Buffer data array field");
    return true;
}

static bool test_parse_expression_precedence() {
    std::string input =
        "int test() {\n"
        "    int res = a + b * c - d / e;\n"
        "    int cond = x < y && a == b || !z;\n"
        "    return res;\n"
        "}\n";

    ParserDriver parser;
    bool ok = parser.parse_string(input);
    TEST_ASSERT(ok, "Expression precedence parsed successfully");
    auto& ast = parser.get_ast();
    auto* func = dynamic_cast<FunctionDefinitionNode*>(ast->declarations[0].get());
    TEST_ASSERT(func && func->body->statements.size() == 3, "Statements in test()");

    // Check res init: (a + (b * c)) - (d / e)
    auto* var0 = dynamic_cast<VariableDeclarationNode*>(func->body->statements[0].get());
    TEST_ASSERT(var0 && var0->initializer != nullptr, "res initializer exists");
    auto* sub = dynamic_cast<BinaryExpressionNode*>(var0->initializer.get());
    TEST_ASSERT(sub && sub->op == BinaryOp::SUB, "Top operator is '-'");
    auto* add = dynamic_cast<BinaryExpressionNode*>(sub->left.get());
    TEST_ASSERT(add && add->op == BinaryOp::ADD, "Left of '-' is '+'");
    auto* mul = dynamic_cast<BinaryExpressionNode*>(add->right.get());
    TEST_ASSERT(mul && mul->op == BinaryOp::MUL, "Right of '+' is '*'");
    return true;
}

static bool test_parse_control_flow() {
    std::string input =
        "int loop(int n) {\n"
        "    int sum = 0;\n"
        "    if (n <= 0) {\n"
        "        return 0;\n"
        "    } else {\n"
        "        sum = 1;\n"
        "    }\n"
        "    while (sum < n) {\n"
        "        sum = sum + 1;\n"
        "    }\n"
        "    for (int i = 0; i < n; i = i + 1) {\n"
        "        sum = sum + i;\n"
        "    }\n"
        "    return sum;\n"
        "}\n";

    ParserDriver parser;
    bool ok = parser.parse_string(input);
    TEST_ASSERT(ok, "Control flow constructs parsed successfully");
    auto& ast = parser.get_ast();
    auto* func = dynamic_cast<FunctionDefinitionNode*>(ast->declarations[0].get());
    TEST_ASSERT(func && func->body->statements.size() == 5, "Statements in loop()");

    auto* if_stmt = dynamic_cast<IfStatementNode*>(func->body->statements[1].get());
    TEST_ASSERT(if_stmt && if_stmt->else_branch != nullptr, "if-else branch present");

    auto* while_stmt = dynamic_cast<WhileStatementNode*>(func->body->statements[2].get());
    TEST_ASSERT(while_stmt && while_stmt->condition != nullptr, "while loop present");

    auto* for_stmt = dynamic_cast<ForStatementNode*>(func->body->statements[3].get());
    TEST_ASSERT(for_stmt && for_stmt->init != nullptr && for_stmt->condition != nullptr, "for loop present");
    return true;
}

static bool test_parse_sample_program() {
    std::string input = 
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

    ParserDriver parser;
    bool ok = parser.parse_string(input, "sample.tc");
    TEST_ASSERT(ok, "Sample program parsed without errors");
    TEST_ASSERT(!parser.has_errors(), "Zero parser errors");
    auto& ast = parser.get_ast();
    TEST_ASSERT(ast->declarations.size() == 3, "Class Point, func add, func main");
    return true;
}

static bool test_negative_syntax_errors() {
    // Missing semicolon
    {
        std::string input = "int main() { int x = 10 return x; }";
        ParserDriver parser;
        bool ok = parser.parse_string(input, "missing_semi.tc");
        TEST_ASSERT(!ok, "Missing semicolon correctly rejected");
        TEST_ASSERT(parser.has_errors(), "Parser recorded syntax error");
        TEST_ASSERT(!parser.get_error_messages().empty(), "Error message contains details");
    }

    // Unbalanced braces
    {
        std::string input = "int main() { int x = 10;";
        ParserDriver parser;
        bool ok = parser.parse_string(input, "unbalanced.tc");
        TEST_ASSERT(!ok, "Unbalanced braces rejected");
        TEST_ASSERT(parser.has_errors(), "Parser recorded error for unclosed brace");
    }

    // Invalid expression syntax (binary operator sequence)
    {
        std::string input = "int main() { int x = 5 % / 2; }";
        ParserDriver parser;
        bool ok = parser.parse_string(input, "invalid_expr.tc");
        TEST_ASSERT(!ok, "Invalid expression syntax rejected");
        TEST_ASSERT(parser.has_errors(), "Parser recorded error for invalid operator sequence");
    }

    return true;
}

int main() {
    int passed = 0;
    int failed = 0;

    std::cout << "========================================\n";
    std::cout << "TACCo Phase 2: Parser & AST Unit Tests\n";
    std::cout << "========================================\n";

    RUN_TEST(test_parse_empty);
    RUN_TEST(test_parse_variable_declarations);
    RUN_TEST(test_parse_functions);
    RUN_TEST(test_parse_classes);
    RUN_TEST(test_parse_expression_precedence);
    RUN_TEST(test_parse_control_flow);
    RUN_TEST(test_parse_sample_program);
    RUN_TEST(test_negative_syntax_errors);

    std::cout << "========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";

    return (failed == 0) ? 0 : 1;
}
