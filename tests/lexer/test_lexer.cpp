#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include "../../src/lexer/lexer_driver.hpp"

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

static bool test_keywords() {
    std::string input = "int char void if else while for return class";
    LexerDriver driver;
    driver.set_input_string(input);
    auto tokens = driver.tokenize_all();

    TEST_ASSERT(tokens.size() == 10, "Expected 9 keywords + 1 EOF");
    TEST_ASSERT(tokens[0].type == TokenType::KW_INT, "Expected KW_INT");
    TEST_ASSERT(tokens[1].type == TokenType::KW_CHAR, "Expected KW_CHAR");
    TEST_ASSERT(tokens[2].type == TokenType::KW_VOID, "Expected KW_VOID");
    TEST_ASSERT(tokens[3].type == TokenType::KW_IF, "Expected KW_IF");
    TEST_ASSERT(tokens[4].type == TokenType::KW_ELSE, "Expected KW_ELSE");
    TEST_ASSERT(tokens[5].type == TokenType::KW_WHILE, "Expected KW_WHILE");
    TEST_ASSERT(tokens[6].type == TokenType::KW_FOR, "Expected KW_FOR");
    TEST_ASSERT(tokens[7].type == TokenType::KW_RETURN, "Expected KW_RETURN");
    TEST_ASSERT(tokens[8].type == TokenType::KW_CLASS, "Expected KW_CLASS");
    TEST_ASSERT(tokens[9].type == TokenType::END_OF_FILE, "Expected EOF");
    return true;
}

static bool test_identifiers_and_numbers() {
    std::string input = "foo _bar123 Point 42 0x2A 0";
    LexerDriver driver;
    driver.set_input_string(input);
    auto tokens = driver.tokenize_all();

    TEST_ASSERT(tokens.size() == 7, "Expected 6 tokens + 1 EOF");
    TEST_ASSERT(tokens[0].type == TokenType::IDENTIFIER && tokens[0].lexeme == "foo", "foo id");
    TEST_ASSERT(tokens[1].type == TokenType::IDENTIFIER && tokens[1].lexeme == "_bar123", "_bar123 id");
    TEST_ASSERT(tokens[2].type == TokenType::IDENTIFIER && tokens[2].lexeme == "Point", "Point id");
    TEST_ASSERT(tokens[3].type == TokenType::INTEGER_LITERAL && tokens[3].int_val == 42, "42 literal");
    TEST_ASSERT(tokens[4].type == TokenType::INTEGER_LITERAL && tokens[4].int_val == 42, "0x2A literal");
    TEST_ASSERT(tokens[5].type == TokenType::INTEGER_LITERAL && tokens[5].int_val == 0, "0 literal");
    return true;
}

static bool test_character_and_string_literals() {
    std::string input = "'a' '\\n' '\\0' '\\'' \"hello world\" \"line1\\nline2\"";
    LexerDriver driver;
    driver.set_input_string(input);
    auto tokens = driver.tokenize_all();

    TEST_ASSERT(tokens.size() == 7, "Expected 6 literals + 1 EOF");
    TEST_ASSERT(tokens[0].type == TokenType::CHAR_LITERAL && tokens[0].char_val == 'a', "'a'");
    TEST_ASSERT(tokens[1].type == TokenType::CHAR_LITERAL && tokens[1].char_val == '\n', "'\\n'");
    TEST_ASSERT(tokens[2].type == TokenType::CHAR_LITERAL && tokens[2].char_val == '\0', "'\\0'");
    TEST_ASSERT(tokens[3].type == TokenType::CHAR_LITERAL && tokens[3].char_val == '\'', "'\\''");
    TEST_ASSERT(tokens[4].type == TokenType::STRING_LITERAL && tokens[4].str_val == "hello world", "string");
    TEST_ASSERT(tokens[5].type == TokenType::STRING_LITERAL && tokens[5].str_val == "line1\nline2", "escaped string");
    return true;
}

static bool test_operators_and_punctuation() {
    std::string input = "+ - * / % == != < > <= >= && || ! = & . ( ) { } [ ] ; ,";
    LexerDriver driver;
    driver.set_input_string(input);
    auto tokens = driver.tokenize_all();

    std::vector<TokenType> expected = {
        TokenType::PLUS, TokenType::MINUS, TokenType::STAR, TokenType::SLASH, TokenType::PERCENT,
        TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL, TokenType::LESS, TokenType::GREATER,
        TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL, TokenType::AND_AND, TokenType::OR_OR,
        TokenType::BANG, TokenType::EQUAL, TokenType::AMPERSAND, TokenType::DOT,
        TokenType::LPAREN, TokenType::RPAREN, TokenType::LBRACE, TokenType::RBRACE,
        TokenType::LBRACKET, TokenType::RBRACKET, TokenType::SEMICOLON, TokenType::COMMA,
        TokenType::END_OF_FILE
    };

    TEST_ASSERT(tokens.size() == expected.size(), "Token count matches operator list");
    for (size_t i = 0; i < expected.size(); ++i) {
        TEST_ASSERT(tokens[i].type == expected[i], "Matching operator token");
    }
    return true;
}

static bool test_comments_and_whitespace() {
    std::string input = "// Line comment\n"
                        "int a = 10; // inline comment\n"
                        "/* multi\n"
                        "   line\n"
                        "   comment */\n"
                        "char c = 'x';";
    LexerDriver driver;
    driver.set_input_string(input);
    auto tokens = driver.tokenize_all();

    TEST_ASSERT(tokens.size() == 11, "Tokens stripped comments correctly");
    TEST_ASSERT(tokens[0].type == TokenType::KW_INT, "int");
    TEST_ASSERT(tokens[1].type == TokenType::IDENTIFIER && tokens[1].lexeme == "a", "a");
    TEST_ASSERT(tokens[2].type == TokenType::EQUAL, "=");
    TEST_ASSERT(tokens[3].type == TokenType::INTEGER_LITERAL && tokens[3].int_val == 10, "10");
    TEST_ASSERT(tokens[4].type == TokenType::SEMICOLON, ";");
    TEST_ASSERT(tokens[5].type == TokenType::KW_CHAR, "char");
    TEST_ASSERT(tokens[6].type == TokenType::IDENTIFIER && tokens[6].lexeme == "c", "c");
    TEST_ASSERT(tokens[7].type == TokenType::EQUAL, "=");
    TEST_ASSERT(tokens[8].type == TokenType::CHAR_LITERAL && tokens[8].char_val == 'x', "'x'");
    TEST_ASSERT(tokens[9].type == TokenType::SEMICOLON, ";");
    TEST_ASSERT(tokens[10].type == TokenType::END_OF_FILE, "EOF");
    return true;
}

static bool test_location_tracking() {
    std::string input = "int x;\n"
                        "  y = 20;";
    LexerDriver driver;
    driver.set_input_string(input, "test.tc");
    auto tokens = driver.tokenize_all();

    // line 1: int (col 1), x (col 5), ; (col 6)
    TEST_ASSERT(tokens[0].location.line == 1 && tokens[0].location.column == 1, "int loc");
    TEST_ASSERT(tokens[1].location.line == 1 && tokens[1].location.column == 5, "x loc");
    TEST_ASSERT(tokens[2].location.line == 1 && tokens[2].location.column == 6, "; loc");

    // line 2: y (col 3), = (col 5), 20 (col 7), ; (col 9)
    TEST_ASSERT(tokens[3].location.line == 2 && tokens[3].location.column == 3, "y loc");
    TEST_ASSERT(tokens[4].location.line == 2 && tokens[4].location.column == 5, "= loc");
    TEST_ASSERT(tokens[5].location.line == 2 && tokens[5].location.column == 7, "20 loc");
    TEST_ASSERT(tokens[6].location.line == 2 && tokens[6].location.column == 9, "; loc");
    return true;
}

static bool test_invalid_characters_and_diagnostics() {
    std::string input = "int @ # $ 123";
    LexerDriver driver;
    driver.set_input_string(input);
    auto tokens = driver.tokenize_all();

    TEST_ASSERT(driver.has_errors(), "Lexer flagged errors");
    TEST_ASSERT(driver.get_error_messages().size() == 3, "Recorded 3 errors for @, #, $");
    TEST_ASSERT(tokens[1].type == TokenType::TOKEN_ERROR, "@ is error");
    TEST_ASSERT(tokens[2].type == TokenType::TOKEN_ERROR, "# is error");
    TEST_ASSERT(tokens[3].type == TokenType::TOKEN_ERROR, "$ is error");
    TEST_ASSERT(tokens[4].type == TokenType::INTEGER_LITERAL, "123 is valid after error");
    return true;
}

static bool test_sample_program() {
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

    LexerDriver driver;
    driver.set_input_string(input, "demo.tc");
    auto tokens = driver.tokenize_all();

    TEST_ASSERT(!driver.has_errors(), "Sample program lexes with zero errors");
    TEST_ASSERT(tokens.size() > 50, "Sample program produces complete token stream");
    TEST_ASSERT(tokens.back().type == TokenType::END_OF_FILE, "Ends with EOF");
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;

    std::cout << "========================================\n";
    std::cout << "TACCo Phase 1: Lexer Unit Test Suite\n";
    std::cout << "========================================\n";

    RUN_TEST(test_keywords);
    RUN_TEST(test_identifiers_and_numbers);
    RUN_TEST(test_character_and_string_literals);
    RUN_TEST(test_operators_and_punctuation);
    RUN_TEST(test_comments_and_whitespace);
    RUN_TEST(test_location_tracking);
    RUN_TEST(test_invalid_characters_and_diagnostics);
    RUN_TEST(test_sample_program);

    std::cout << "========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";

    return (failed == 0) ? 0 : 1;
}
