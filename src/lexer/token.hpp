#ifndef TACCO_TOKEN_HPP
#define TACCO_TOKEN_HPP

#include <string>
#include <cstdint>
#include "../common/location.hpp"

namespace tacco {

enum class TokenType {
    // End of file & Error
    END_OF_FILE,
    TOKEN_ERROR,

    // Keywords
    KW_INT,
    KW_CHAR,
    KW_VOID,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FOR,
    KW_RETURN,
    KW_CLASS,

    // Identifiers & Literals
    IDENTIFIER,
    INTEGER_LITERAL,
    CHAR_LITERAL,
    STRING_LITERAL,

    // Arithmetic Operators
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    PERCENT,        // %

    // Relational / Comparison Operators
    EQUAL_EQUAL,    // ==
    BANG_EQUAL,     // !=
    LESS,           // <
    GREATER,        // >
    LESS_EQUAL,     // <=
    GREATER_EQUAL,  // >=

    // Logical Operators
    AND_AND,        // &&
    OR_OR,          // ||
    BANG,           // !

    // Assignment & Memory/Pointer Operators
    EQUAL,          // =
    AMPERSAND,      // &
    DOT,            // .

    // Punctuation / Delimiters
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]
    SEMICOLON,      // ;
    COMMA           // ,
};

const char* token_type_to_string(TokenType type);

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation location;

    int64_t int_val = 0;
    char char_val = '\0';
    std::string str_val = "";
    std::string error_msg = "";

    Token(TokenType t, std::string lex, SourceLocation loc)
        : type(t), lexeme(std::move(lex)), location(std::move(loc)) {}

    std::string to_string() const;
};

} // namespace tacco

#endif // TACCO_TOKEN_HPP
