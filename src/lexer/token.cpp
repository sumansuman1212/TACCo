#include "token.hpp"
#include <sstream>
#include <iomanip>

namespace tacco {

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE:    return "EOF";
        case TokenType::TOKEN_ERROR:    return "ERROR";

        case TokenType::KW_INT:         return "int";
        case TokenType::KW_CHAR:        return "char";
        case TokenType::KW_VOID:        return "void";
        case TokenType::KW_IF:          return "if";
        case TokenType::KW_ELSE:        return "else";
        case TokenType::KW_WHILE:       return "while";
        case TokenType::KW_FOR:         return "for";
        case TokenType::KW_RETURN:      return "return";
        case TokenType::KW_CLASS:       return "class";

        case TokenType::IDENTIFIER:      return "IDENTIFIER";
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::CHAR_LITERAL:    return "CHAR_LITERAL";
        case TokenType::STRING_LITERAL:  return "STRING_LITERAL";

        case TokenType::PLUS:           return "+";
        case TokenType::MINUS:          return "-";
        case TokenType::STAR:           return "*";
        case TokenType::SLASH:          return "/";
        case TokenType::PERCENT:        return "%";

        case TokenType::EQUAL_EQUAL:    return "==";
        case TokenType::BANG_EQUAL:     return "!=";
        case TokenType::LESS:           return "<";
        case TokenType::GREATER:        return ">";
        case TokenType::LESS_EQUAL:     return "<=";
        case TokenType::GREATER_EQUAL:  return ">=";

        case TokenType::AND_AND:        return "&&";
        case TokenType::OR_OR:          return "||";
        case TokenType::BANG:           return "!";

        case TokenType::EQUAL:          return "=";
        case TokenType::AMPERSAND:      return "&";
        case TokenType::DOT:            return ".";

        case TokenType::LPAREN:         return "(";
        case TokenType::RPAREN:         return ")";
        case TokenType::LBRACE:         return "{";
        case TokenType::RBRACE:         return "}";
        case TokenType::LBRACKET:       return "[";
        case TokenType::RBRACKET:       return "]";
        case TokenType::SEMICOLON:      return ";";
        case TokenType::COMMA:          return ",";
    }
    return "UNKNOWN";
}

std::string Token::to_string() const {
    std::ostringstream oss;
    oss << "[" << location.to_string() << "] "
        << std::left << std::setw(16) << token_type_to_string(type)
        << " '" << lexeme << "'";

    if (type == TokenType::INTEGER_LITERAL) {
        oss << " (val=" << int_val << ")";
    } else if (type == TokenType::CHAR_LITERAL) {
        oss << " (val=" << static_cast<int>(char_val) << " '" << char_val << "')";
    } else if (type == TokenType::STRING_LITERAL) {
        oss << " (val=\"" << str_val << "\")";
    } else if (type == TokenType::TOKEN_ERROR) {
        oss << " (msg=\"" << error_msg << "\")";
    }

    return oss.str();
}

} // namespace tacco
