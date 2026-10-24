#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include "../../src/semantic/symbol.hpp"
#include "../../src/semantic/scope.hpp"
#include "../../src/semantic/symbol_table.hpp"
#include "../../src/parser/parser_driver.hpp"

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

static bool test_basic_scope_and_lookup() {
    auto global_scope = std::make_shared<Scope>(ScopeKind::GLOBAL, nullptr, "global");
    auto sym_a = Symbol::make_variable("a", Type::make_int(), SourceLocation("test.tc", 1, 1));
    auto sym_b = Symbol::make_variable("b", Type::make_char(), SourceLocation("test.tc", 2, 1));

    TEST_ASSERT(global_scope->insert(sym_a), "Inserted a");
    TEST_ASSERT(global_scope->insert(sym_b), "Inserted b");

    auto found_a = global_scope->lookup("a");
    TEST_ASSERT(found_a && found_a->type->is_int(), "Found a in global");
    auto found_b = global_scope->lookup("b");
    TEST_ASSERT(found_b && found_b->type->is_char(), "Found b in global");
    auto found_c = global_scope->lookup("c");
    TEST_ASSERT(found_c == nullptr, "c not in scope");
    return true;
}

static bool test_nested_scopes_and_resolution() {
    auto global = std::make_shared<Scope>(ScopeKind::GLOBAL, nullptr, "global");
    auto fn_scope = global->add_child(ScopeKind::FUNCTION, "func");
    auto block_scope = fn_scope->add_child(ScopeKind::BLOCK, "block");

    global->insert(Symbol::make_variable("global_var", Type::make_int(), SourceLocation("t.tc", 1, 1)));
    fn_scope->insert(Symbol::make_variable("fn_var", Type::make_int(), SourceLocation("t.tc", 2, 1)));
    block_scope->insert(Symbol::make_variable("local_var", Type::make_int(), SourceLocation("t.tc", 3, 1)));

    // From block scope: should find all three
    TEST_ASSERT(block_scope->lookup("local_var") != nullptr, "Found local_var in block");
    TEST_ASSERT(block_scope->lookup("fn_var") != nullptr, "Found fn_var in block");
    TEST_ASSERT(block_scope->lookup("global_var") != nullptr, "Found global_var in block");

    // From fn_scope: should find fn_var and global_var, but NOT local_var
    TEST_ASSERT(fn_scope->lookup("local_var") == nullptr, "local_var not visible in fn_scope");
    TEST_ASSERT(fn_scope->lookup("fn_var") != nullptr, "fn_var visible in fn_scope");
    TEST_ASSERT(fn_scope->lookup("global_var") != nullptr, "global_var visible in fn_scope");

    // From global: only global_var visible
    TEST_ASSERT(global->lookup("local_var") == nullptr, "local_var not visible in global");
    TEST_ASSERT(global->lookup("fn_var") == nullptr, "fn_var not visible in global");
    TEST_ASSERT(global->lookup("global_var") != nullptr, "global_var visible in global");
    return true;
}

static bool test_variable_shadowing() {
    auto global = std::make_shared<Scope>(ScopeKind::GLOBAL, nullptr, "global");
    auto block = global->add_child(ScopeKind::BLOCK, "block");

    auto outer_x = Symbol::make_variable("x", Type::make_int(), SourceLocation("t.tc", 1, 1));
    auto inner_x = Symbol::make_variable("x", Type::make_char(), SourceLocation("t.tc", 2, 1));

    global->insert(outer_x);
    block->insert(inner_x);

    // In block scope, lookup resolves to inner_x (type char)
    auto found_in_block = block->lookup("x");
    TEST_ASSERT(found_in_block && found_in_block->type->is_char(), "Block scope sees inner shadow x");

    // In global scope, lookup resolves to outer_x (type int)
    auto found_in_global = global->lookup("x");
    TEST_ASSERT(found_in_global && found_in_global->type->is_int(), "Global scope sees outer x");
    return true;
}

static bool test_duplicate_declarations_rejected() {
    SymbolTable symtab;
    auto sym1 = Symbol::make_variable("x", Type::make_int(), SourceLocation("t.tc", 1, 1));
    auto sym2 = Symbol::make_variable("x", Type::make_char(), SourceLocation("t.tc", 2, 1));

    TEST_ASSERT(symtab.insert(sym1), "First insert of x succeeds");
    TEST_ASSERT(!symtab.insert(sym2), "Duplicate insert of x in same scope fails");
    TEST_ASSERT(symtab.has_errors(), "Error recorded for duplicate symbol");
    return true;
}

static bool test_class_and_fields() {
    auto class_sym = Symbol::make_class("Point", SourceLocation("t.tc", 1, 1));
    auto f_x = Symbol::make_field("x", Type::make_int(), SourceLocation("t.tc", 2, 5));
    auto f_y = Symbol::make_field("y", Type::make_int(), SourceLocation("t.tc", 3, 5));

    TEST_ASSERT(class_sym->add_field(f_x), "Added field x");
    TEST_ASSERT(class_sym->add_field(f_y), "Added field y");
    TEST_ASSERT(!class_sym->add_field(f_x), "Duplicate field x rejected");

    TEST_ASSERT(class_sym->get_field("x") != nullptr, "Lookup field x");
    TEST_ASSERT(class_sym->get_field("y") != nullptr, "Lookup field y");
    TEST_ASSERT(class_sym->get_field("z") == nullptr, "Lookup nonexistent field z");
    return true;
}

static bool test_ast_symbol_table_construction() {
    std::string source =
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
        "    Point pt;\n"
        "    {\n"
        "        int block_scoped = 99;\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    ParserDriver parser;
    bool parse_ok = parser.parse_string(source, "demo.tc");
    TEST_ASSERT(parse_ok, "Parsed program");

    SymbolTable symtab;
    bool sym_ok = symtab.build(*parser.get_ast());
    TEST_ASSERT(sym_ok, "Built symbol table without errors");
    TEST_ASSERT(!symtab.has_errors(), "Zero errors");

    auto global = symtab.get_global_scope();
    TEST_ASSERT(global != nullptr, "Global scope exists");

    auto pt_class = global->lookup("Point");
    TEST_ASSERT(pt_class && pt_class->is_class(), "Point class registered");
    TEST_ASSERT(pt_class->get_field("x") != nullptr, "Point has field x");
    TEST_ASSERT(pt_class->get_field("y") != nullptr, "Point has field y");

    auto add_fn = global->lookup("add");
    TEST_ASSERT(add_fn && add_fn->is_function(), "add function registered");
    TEST_ASSERT(add_fn->parameters.size() == 2, "add has 2 parameters");

    auto main_fn = global->lookup("main");
    TEST_ASSERT(main_fn && main_fn->is_function(), "main function registered");
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;

    std::cout << "========================================\n";
    std::cout << "TACCo Phase 4: Symbol Table & Scopes Unit Tests\n";
    std::cout << "========================================\n";

    RUN_TEST(test_basic_scope_and_lookup);
    RUN_TEST(test_nested_scopes_and_resolution);
    RUN_TEST(test_variable_shadowing);
    RUN_TEST(test_duplicate_declarations_rejected);
    RUN_TEST(test_class_and_fields);
    RUN_TEST(test_ast_symbol_table_construction);

    std::cout << "========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";

    return (failed == 0) ? 0 : 1;
}
