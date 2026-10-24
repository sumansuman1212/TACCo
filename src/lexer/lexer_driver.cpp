#include "lexer_driver.hpp"
#include "parser.hpp"
#include <cstdio>
#include <cstdlib>
#include <cerrno>

typedef void* yyscan_t;
typedef struct yy_buffer_state* YY_BUFFER_STATE;

extern int tacco_yylex_init_extra(tacco::LexerDriver* user_defined, yyscan_t* scanner);
extern int tacco_yylex_destroy(yyscan_t yyscanner);
extern void tacco_yyset_in(FILE* in_str, yyscan_t yyscanner);
extern YY_BUFFER_STATE tacco_yy_scan_string(const char* yystr, yyscan_t yyscanner);
extern void tacco_yy_delete_buffer(YY_BUFFER_STATE b, yyscan_t yyscanner);
extern char* tacco_yyget_text(yyscan_t yyscanner);
extern int tacco_yyget_leng(yyscan_t yyscanner);
extern int tacco_yylex(TACCO_YYSTYPE* yylval_param, TACCO_YYLTYPE* yylloc_param, yyscan_t yyscanner);

namespace tacco {

static TokenType map_bison_token_to_type(int tok) {
    switch (tok) {
        case 0: return TokenType::END_OF_FILE;
        case TACCO_YYerror: return TokenType::TOKEN_ERROR;
        case KW_INT: return TokenType::KW_INT;
        case KW_CHAR: return TokenType::KW_CHAR;
        case KW_VOID: return TokenType::KW_VOID;
        case KW_IF: return TokenType::KW_IF;
        case KW_ELSE: return TokenType::KW_ELSE;
        case KW_WHILE: return TokenType::KW_WHILE;
        case KW_FOR: return TokenType::KW_FOR;
        case KW_RETURN: return TokenType::KW_RETURN;
        case KW_CLASS: return TokenType::KW_CLASS;

        case IDENTIFIER: return TokenType::IDENTIFIER;
        case INTEGER_LITERAL: return TokenType::INTEGER_LITERAL;
        case CHAR_LITERAL: return TokenType::CHAR_LITERAL;
        case STRING_LITERAL: return TokenType::STRING_LITERAL;

        case PLUS: return TokenType::PLUS;
        case MINUS: return TokenType::MINUS;
        case STAR: return TokenType::STAR;
        case SLASH: return TokenType::SLASH;
        case PERCENT: return TokenType::PERCENT;

        case EQUAL_EQUAL: return TokenType::EQUAL_EQUAL;
        case BANG_EQUAL: return TokenType::BANG_EQUAL;
        case LESS: return TokenType::LESS;
        case GREATER: return TokenType::GREATER;
        case LESS_EQUAL: return TokenType::LESS_EQUAL;
        case GREATER_EQUAL: return TokenType::GREATER_EQUAL;

        case AND_AND: return TokenType::AND_AND;
        case OR_OR: return TokenType::OR_OR;
        case BANG: return TokenType::BANG;

        case EQUAL: return TokenType::EQUAL;
        case AMPERSAND: return TokenType::AMPERSAND;
        case DOT: return TokenType::DOT;

        case LPAREN: return TokenType::LPAREN;
        case RPAREN: return TokenType::RPAREN;
        case LBRACE: return TokenType::LBRACE;
        case RBRACE: return TokenType::RBRACE;
        case LBRACKET: return TokenType::LBRACKET;
        case RBRACKET: return TokenType::RBRACKET;
        case SEMICOLON: return TokenType::SEMICOLON;
        case COMMA: return TokenType::COMMA;
    }
    return TokenType::TOKEN_ERROR;
}

LexerDriver::LexerDriver() {
    yyscan_t scan;
    tacco_yylex_init_extra(this, &scan);
    scanner_ = scan;
    current_line_ = 1;
    current_column_ = 1;
}

LexerDriver::~LexerDriver() {
    if (buffer_state_) {
        tacco_yy_delete_buffer(reinterpret_cast<YY_BUFFER_STATE>(buffer_state_), scanner_);
        buffer_state_ = nullptr;
    }
    if (file_handle_) {
        fclose(file_handle_);
        file_handle_ = nullptr;
    }
    if (scanner_) {
        tacco_yylex_destroy(scanner_);
        scanner_ = nullptr;
    }
}

bool LexerDriver::open_file(const std::string& filepath) {
    filename_ = filepath;
    current_line_ = 1;
    current_column_ = 1;
    error_messages_.clear();

    if (file_handle_) {
        fclose(file_handle_);
        file_handle_ = nullptr;
    }

    file_handle_ = fopen(filepath.c_str(), "r");
    if (!file_handle_) {
        std::string msg = "Cannot open file '" + filepath + "'";
        record_error(msg, SourceLocation(filepath, 1, 1));
        return false;
    }

    tacco_yyset_in(file_handle_, scanner_);
    return true;
}

void LexerDriver::set_input_string(const std::string& input, const std::string& source_name) {
    filename_ = source_name;
    current_line_ = 1;
    current_column_ = 1;
    error_messages_.clear();
    string_buffer_ = input;

    if (buffer_state_) {
        tacco_yy_delete_buffer(reinterpret_cast<YY_BUFFER_STATE>(buffer_state_), scanner_);
        buffer_state_ = nullptr;
    }

    buffer_state_ = tacco_yy_scan_string(string_buffer_.c_str(), scanner_);
}

void LexerDriver::record_error(const std::string& msg, const SourceLocation& loc) {
    std::string formatted = "error: " + msg + " at " + loc.to_string();
    error_messages_.push_back(formatted);
}

Token LexerDriver::next_token() {
    TACCO_YYSTYPE yylval;
    TACCO_YYLTYPE yylloc;

    int tok_val = tacco_yylex(&yylval, &yylloc, scanner_);
    TokenType type = map_bison_token_to_type(tok_val);

    const char* text_ptr = tacco_yyget_text(scanner_);
    std::string lexeme = text_ptr ? text_ptr : "";
    int len = tacco_yyget_leng(scanner_);

    int start_col = current_column_ - len;
    if (start_col < 1) start_col = 1;
    SourceLocation loc(filename_, current_line_, start_col);

    Token token(type, lexeme, loc);

    if (tok_val == INTEGER_LITERAL) {
        token.int_val = yylval.int_val;
    } else if (tok_val == CHAR_LITERAL) {
        token.char_val = yylval.char_val;
        token.int_val = static_cast<unsigned char>(token.char_val);
    } else if (tok_val == STRING_LITERAL) {
        if (yylval.str_val) {
            token.str_val = *yylval.str_val;
            delete yylval.str_val;
        }
    } else if (tok_val == IDENTIFIER) {
        if (yylval.str_val) {
            token.str_val = *yylval.str_val;
            delete yylval.str_val;
        }
    } else if (type == TokenType::TOKEN_ERROR) {
        if (!error_messages_.empty()) {
            token.error_msg = error_messages_.back();
        } else {
            token.error_msg = "Unknown lexical error";
        }
    }

    return token;
}

std::vector<Token> LexerDriver::tokenize_all() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = next_token();
        tokens.push_back(tok);
        if (tok.type == TokenType::END_OF_FILE) {
            break;
        }
    }
    return tokens;
}

} // namespace tacco
