%top{
#include "parser.hpp"
#define YYSTYPE TACCO_YYSTYPE
#define YYLTYPE TACCO_YYLTYPE
}

%{
#include "lexer/lexer_driver.hpp"
#include "common/location.hpp"
#include <cstdlib>
#include <string>
#include <cerrno>

#define YY_EXTRA_TYPE tacco::LexerDriver*

#define YY_USER_ACTION                                           \
    yylloc->first_line = yyextra->get_line();                    \
    yylloc->first_column = yyextra->get_column();                \
    yyextra->advance_column(yyleng);                             \
    yylloc->last_line = yyextra->get_line();                     \
    yylloc->last_column = yyextra->get_column();

static char parse_escape_char(const char* str) {
    if (str[0] != '\\') return str[0];
    switch (str[1]) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '0': return '\0';
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'v': return '\v';
        case '\\': return '\\';
        case '\'': return '\'';
        case '\"': return '\"';
        default: return str[1];
    }
}

static std::string parse_string_contents(const char* str, int len) {
    std::string result;
    result.reserve(len > 2 ? len - 2 : 0);
    for (int i = 1; i < len - 1; ++i) {
        if (str[i] == '\\' && i + 1 < len - 1) {
            result.push_back(parse_escape_char(&str[i]));
            ++i;
        } else {
            result.push_back(str[i]);
        }
    }
    return result;
}

static char parse_char_literal(const char* str, int len) {
    if (len >= 3) {
        if (str[1] == '\\') {
            return parse_escape_char(&str[1]);
        }
        return str[1];
    }
    return '\0';
}
%}

%option noyywrap
%option nounput
%option noinput
%option reentrant
%option bison-bridge
%option bison-locations
%option extra-type="tacco::LexerDriver*"
%option prefix="tacco_yy"

%x BLOCK_COMMENT

%%

[ \t\r]+ {
    // Whitespace: column is updated by YY_USER_ACTION
}

\n {
    yyextra->newline();
}

"//".* {
    // Single-line comment: ignore until newline
}

"/*" {
    BEGIN(BLOCK_COMMENT);
}

<BLOCK_COMMENT>"*/" {
    BEGIN(INITIAL);
}

<BLOCK_COMMENT>\n {
    yyextra->newline();
}

<BLOCK_COMMENT>. {
    // Ignore characters
}

<BLOCK_COMMENT><<EOF>> {
    tacco::SourceLocation loc(yyextra->get_filename(), yylloc->first_line, yylloc->first_column);
    yyextra->record_error("Unterminated block comment", loc);
    BEGIN(INITIAL);
    return TACCO_YYerror;
}

"int"       { return KW_INT; }
"char"      { return KW_CHAR; }
"void"      { return KW_VOID; }
"if"        { return KW_IF; }
"else"      { return KW_ELSE; }
"while"     { return KW_WHILE; }
"for"       { return KW_FOR; }
"return"    { return KW_RETURN; }
"class"     { return KW_CLASS; }

"=="        { return EQUAL_EQUAL; }
"!="        { return BANG_EQUAL; }
"<="        { return LESS_EQUAL; }
">="        { return GREATER_EQUAL; }
"&&"        { return AND_AND; }
"||"        { return OR_OR; }

"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return STAR; }
"/"         { return SLASH; }
"%"         { return PERCENT; }
"<"         { return LESS; }
">"         { return GREATER; }
"!"         { return BANG; }
"="         { return EQUAL; }
"&"         { return AMPERSAND; }
"."         { return DOT; }

"("         { return LPAREN; }
")"         { return RPAREN; }
"{"         { return LBRACE; }
"}"         { return RBRACE; }
"["         { return LBRACKET; }
"]"         { return RBRACKET; }
";"         { return SEMICOLON; }
","         { return COMMA; }

0[xX][0-9a-fA-F]+ {
    yylval->int_val = std::strtoll(yytext, nullptr, 16);
    return INTEGER_LITERAL;
}

[0-9]+ {
    yylval->int_val = std::strtoll(yytext, nullptr, 10);
    return INTEGER_LITERAL;
}

'([^'\\\n]|\\.)' {
    yylval->char_val = parse_char_literal(yytext, yyleng);
    return CHAR_LITERAL;
}

'([^'\\\n]|\\.)* {
    tacco::SourceLocation loc(yyextra->get_filename(), yylloc->first_line, yylloc->first_column);
    yyextra->record_error("Unterminated character literal", loc);
    return TACCO_YYerror;
}

\"([^\"\\\n]|\\.)*\" {
    yylval->str_val = new std::string(parse_string_contents(yytext, yyleng));
    return STRING_LITERAL;
}

\"([^\"\\\n]|\\.)* {
    tacco::SourceLocation loc(yyextra->get_filename(), yylloc->first_line, yylloc->first_column);
    yyextra->record_error("Unterminated string literal", loc);
    return TACCO_YYerror;
}

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval->str_val = new std::string(yytext);
    return IDENTIFIER;
}

<<EOF>> {
    return 0;
}

. {
    tacco::SourceLocation loc(yyextra->get_filename(), yylloc->first_line, yylloc->first_column);
    std::string msg = "Unexpected character '";
    msg += yytext;
    msg += "'";
    yyextra->record_error(msg, loc);
    return TACCO_YYerror;
}

%%
