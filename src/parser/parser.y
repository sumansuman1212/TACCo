%{
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
%}

%code requires {
#include <string>
#include <vector>
#include <memory>
#include "common/location.hpp"
#include "ast/ast.hpp"
#include "ast/types.hpp"
#include "ast/expressions.hpp"
#include "ast/statements.hpp"
#include "ast/declarations.hpp"

namespace tacco {
    class ParserDriver;
}

typedef void* yyscan_t;
}

%code {
#include "parser/parser_driver.hpp"

// Forward declaration of lexer function
int tacco_yylex(YYSTYPE* yylval_param, YYLTYPE* yylloc_param, yyscan_t yyscanner);

void tacco_yyerror(YYLTYPE* yylloc, tacco::ParserDriver& driver, yyscan_t yyscanner, const char* msg) {
    (void)yyscanner;
    tacco::SourceLocation loc(driver.get_filename(), yylloc->first_line, yylloc->first_column);
    driver.record_error(msg, loc);
}

#define MAKE_LOC(yyloc) tacco::SourceLocation(driver.get_filename(), (yyloc).first_line, (yyloc).first_column)
}

%define api.prefix {tacco_yy}
%define api.pure full
%define parse.error verbose
// Note on LALR(1) reduce/reduce conflict:
// On token '*', an identifier could begin a pointer declaration (type_specifier -> IDENTIFIER)
// or an expression statement (primary_expression -> IDENTIFIER). In LALR(1), Bison defaults
// to the earlier reduction rule (type_specifier), correctly prioritizing declarations.
%locations

%lex-param { void* scanner }
%parse-param { tacco::ParserDriver& driver }
%parse-param { void* scanner }

%union {
    int64_t int_val;
    char char_val;
    std::string* str_val;
    tacco::Type* type_val;
    tacco::ExpressionNode* expr_val;
    tacco::StatementNode* stmt_val;
    tacco::CompoundStatementNode* compound_val;
    tacco::DeclarationNode* decl_val;
    tacco::ParameterNode* param_val;
    tacco::FieldDeclarationNode* field_val;
    std::vector<std::unique_ptr<tacco::ExpressionNode>>* expr_list_val;
    std::vector<std::unique_ptr<tacco::StatementNode>>* stmt_list_val;
    std::vector<std::unique_ptr<tacco::ParameterNode>>* param_list_val;
    std::vector<std::unique_ptr<tacco::FieldDeclarationNode>>* field_list_val;
    std::vector<std::unique_ptr<tacco::DeclarationNode>>* decl_list_val;
}

/* Tokens */
%token KW_INT "int"
%token KW_CHAR "char"
%token KW_VOID "void"
%token KW_IF "if"
%token KW_ELSE "else"
%token KW_WHILE "while"
%token KW_FOR "for"
%token KW_RETURN "return"
%token KW_CLASS "class"

%token <str_val> IDENTIFIER "identifier"
%token <int_val> INTEGER_LITERAL "integer literal"
%token <char_val> CHAR_LITERAL "character literal"
%token <str_val> STRING_LITERAL "string literal"

%token PLUS "+"
%token MINUS "-"
%token STAR "*"
%token SLASH "/"
%token PERCENT "%"

%token EQUAL_EQUAL "=="
%token BANG_EQUAL "!="
%token LESS "<"
%token GREATER ">"
%token LESS_EQUAL "<="
%token GREATER_EQUAL ">="

%token AND_AND "&&"
%token OR_OR "||"
%token BANG "!"

%token EQUAL "="
%token AMPERSAND "&"
%token DOT "."

%token LPAREN "("
%token RPAREN ")"
%token LBRACE "{"
%token RBRACE "}"
%token LBRACKET "["
%token RBRACKET "]"
%token SEMICOLON ";"
%token COMMA ","

/* Precedence and Associativity */
%right EQUAL
%left OR_OR
%left AND_AND
%left EQUAL_EQUAL BANG_EQUAL
%left LESS GREATER LESS_EQUAL GREATER_EQUAL
%left PLUS MINUS
%left STAR SLASH PERCENT
%right UMINUS UPLUS BANG UNARY_DEREF UNARY_ADDR
%left LBRACKET LPAREN DOT

%nonassoc LOWER_THAN_ELSE
%nonassoc KW_ELSE

/* Non-terminal types */
%type <type_val> type type_specifier
%type <int_val> pointer_stars
%type <expr_val> expression primary_expression postfix_expression unary_expression assignment_expression
%type <expr_val> optional_expression
%type <expr_list_val> argument_list argument_list_opt
%type <stmt_val> statement expression_statement if_statement while_statement for_statement return_statement
%type <stmt_val> for_init
%type <compound_val> compound_statement
%type <stmt_list_val> statement_list
%type <decl_val> top_level_declaration variable_declaration function_definition function_declaration class_declaration
%type <decl_list_val> top_level_declaration_list
%type <param_val> parameter
%type <param_list_val> parameter_list parameter_list_opt
%type <field_val> field_declaration
%type <field_list_val> field_declaration_list field_declaration_list_opt

%start program

%%

program
    : top_level_declaration_list {
        auto prog = std::make_unique<tacco::ProgramNode>(MAKE_LOC(@$));
        if ($1) {
            for (auto& decl : *$1) {
                prog->add_declaration(std::move(decl));
            }
            delete $1;
        }
        driver.set_ast(std::move(prog));
    }
    | /* empty program */ {
        auto prog = std::make_unique<tacco::ProgramNode>(MAKE_LOC(@$));
        driver.set_ast(std::move(prog));
    }
    ;

top_level_declaration_list
    : top_level_declaration {
        $$ = new std::vector<std::unique_ptr<tacco::DeclarationNode>>();
        if ($1) $$->push_back(std::unique_ptr<tacco::DeclarationNode>($1));
    }
    | top_level_declaration_list top_level_declaration {
        $$ = $1;
        if ($2) $$->push_back(std::unique_ptr<tacco::DeclarationNode>($2));
    }
    ;

top_level_declaration
    : variable_declaration { $$ = $1; }
    | function_definition { $$ = $1; }
    | function_declaration { $$ = $1; }
    | class_declaration { $$ = $1; }
    ;

/* Types */
type_specifier
    : KW_INT    { $$ = new tacco::Type(tacco::TypeKind::INT, "int"); }
    | KW_CHAR   { $$ = new tacco::Type(tacco::TypeKind::CHAR, "char"); }
    | KW_VOID   { $$ = new tacco::Type(tacco::TypeKind::VOID, "void"); }
    | IDENTIFIER {
        $$ = new tacco::Type(tacco::TypeKind::CLASS, *$1);
        delete $1;
    }
    ;

pointer_stars
    : STAR { $$ = 1; }
    | pointer_stars STAR { $$ = $1 + 1; }
    ;

type
    : type_specifier {
        $$ = $1;
    }
    | type_specifier pointer_stars {
        tacco::TypePtr base = std::shared_ptr<tacco::Type>($1);
        for (int i = 0; i < $2; ++i) {
            base = tacco::Type::make_pointer(base);
        }
        $$ = new tacco::Type(*base);
    }
    ;

/* Variable Declarations */
variable_declaration
    : type IDENTIFIER SEMICOLON {
        tacco::TypePtr t = std::shared_ptr<tacco::Type>($1);
        $$ = new tacco::VariableDeclarationNode(t, *$2, nullptr, 0, false, MAKE_LOC(@$));
        delete $2;
    }
    | type IDENTIFIER EQUAL expression SEMICOLON {
        tacco::TypePtr t = std::shared_ptr<tacco::Type>($1);
        $$ = new tacco::VariableDeclarationNode(t, *$2, std::unique_ptr<tacco::ExpressionNode>($4), 0, false, MAKE_LOC(@$));
        delete $2;
    }
    | type IDENTIFIER LBRACKET INTEGER_LITERAL RBRACKET SEMICOLON {
        tacco::TypePtr t = std::shared_ptr<tacco::Type>($1);
        int arr_size = static_cast<int>($4);
        $$ = new tacco::VariableDeclarationNode(t, *$2, nullptr, arr_size, true, MAKE_LOC(@$));
        delete $2;
    }
    ;

/* Function Declarations and Definitions */
parameter
    : type IDENTIFIER {
        tacco::TypePtr t = std::shared_ptr<tacco::Type>($1);
        $$ = new tacco::ParameterNode(t, *$2, MAKE_LOC(@$));
        delete $2;
    }
    ;

parameter_list
    : parameter {
        $$ = new std::vector<std::unique_ptr<tacco::ParameterNode>>();
        $$->push_back(std::unique_ptr<tacco::ParameterNode>($1));
    }
    | parameter_list COMMA parameter {
        $$ = $1;
        $$->push_back(std::unique_ptr<tacco::ParameterNode>($3));
    }
    ;

parameter_list_opt
    : /* empty */ { $$ = new std::vector<std::unique_ptr<tacco::ParameterNode>>(); }
    | parameter_list { $$ = $1; }
    ;

function_declaration
    : type IDENTIFIER LPAREN parameter_list_opt RPAREN SEMICOLON {
        tacco::TypePtr ret = std::shared_ptr<tacco::Type>($1);
        std::vector<std::unique_ptr<tacco::ParameterNode>> params;
        if ($4) {
            params = std::move(*$4);
            delete $4;
        }
        $$ = new tacco::FunctionDeclarationNode(ret, *$2, std::move(params), MAKE_LOC(@$));
        delete $2;
    }
    ;

function_definition
    : type IDENTIFIER LPAREN parameter_list_opt RPAREN compound_statement {
        tacco::TypePtr ret = std::shared_ptr<tacco::Type>($1);
        std::vector<std::unique_ptr<tacco::ParameterNode>> params;
        if ($4) {
            params = std::move(*$4);
            delete $4;
        }
        std::unique_ptr<tacco::CompoundStatementNode> body($6);
        $$ = new tacco::FunctionDefinitionNode(ret, *$2, std::move(params), std::move(body), MAKE_LOC(@$));
        delete $2;
    }
    ;

/* Class Declarations */
field_declaration
    : type IDENTIFIER SEMICOLON {
        tacco::TypePtr t = std::shared_ptr<tacco::Type>($1);
        $$ = new tacco::FieldDeclarationNode(t, *$2, 0, false, MAKE_LOC(@$));
        delete $2;
    }
    | type IDENTIFIER LBRACKET INTEGER_LITERAL RBRACKET SEMICOLON {
        tacco::TypePtr t = std::shared_ptr<tacco::Type>($1);
        $$ = new tacco::FieldDeclarationNode(t, *$2, static_cast<int>($4), true, MAKE_LOC(@$));
        delete $2;
    }
    ;

field_declaration_list
    : field_declaration {
        $$ = new std::vector<std::unique_ptr<tacco::FieldDeclarationNode>>();
        $$->push_back(std::unique_ptr<tacco::FieldDeclarationNode>($1));
    }
    | field_declaration_list field_declaration {
        $$ = $1;
        $$->push_back(std::unique_ptr<tacco::FieldDeclarationNode>($2));
    }
    ;

field_declaration_list_opt
    : /* empty */ { $$ = new std::vector<std::unique_ptr<tacco::FieldDeclarationNode>>(); }
    | field_declaration_list { $$ = $1; }
    ;

class_declaration
    : KW_CLASS IDENTIFIER LBRACE field_declaration_list_opt RBRACE SEMICOLON {
        std::vector<std::unique_ptr<tacco::FieldDeclarationNode>> fields;
        if ($4) {
            fields = std::move(*$4);
            delete $4;
        }
        $$ = new tacco::ClassDeclarationNode(*$2, std::move(fields), MAKE_LOC(@$));
        delete $2;
    }
    ;

/* Statements */
statement_list
    : statement {
        $$ = new std::vector<std::unique_ptr<tacco::StatementNode>>();
        if ($1) $$->push_back(std::unique_ptr<tacco::StatementNode>($1));
    }
    | statement_list statement {
        $$ = $1;
        if ($2) $$->push_back(std::unique_ptr<tacco::StatementNode>($2));
    }
    ;

compound_statement
    : LBRACE statement_list RBRACE {
        std::vector<std::unique_ptr<tacco::StatementNode>> stmts;
        if ($2) {
            stmts = std::move(*$2);
            delete $2;
        }
        $$ = new tacco::CompoundStatementNode(std::move(stmts), MAKE_LOC(@$));
    }
    | LBRACE RBRACE {
        $$ = new tacco::CompoundStatementNode(MAKE_LOC(@$));
    }
    ;

statement
    : expression_statement { $$ = $1; }
    | compound_statement { $$ = $1; }
    | if_statement { $$ = $1; }
    | while_statement { $$ = $1; }
    | for_statement { $$ = $1; }
    | return_statement { $$ = $1; }
    | variable_declaration { $$ = $1; }
    ;

expression_statement
    : expression SEMICOLON {
        $$ = new tacco::ExpressionStatementNode(std::unique_ptr<tacco::ExpressionNode>($1), MAKE_LOC(@$));
    }
    | SEMICOLON {
        $$ = new tacco::ExpressionStatementNode(nullptr, MAKE_LOC(@$));
    }
    ;

if_statement
    : KW_IF LPAREN expression RPAREN statement %prec LOWER_THAN_ELSE {
        $$ = new tacco::IfStatementNode(std::unique_ptr<tacco::ExpressionNode>($3),
                                        std::unique_ptr<tacco::StatementNode>($5),
                                        nullptr, MAKE_LOC(@$));
    }
    | KW_IF LPAREN expression RPAREN statement KW_ELSE statement {
        $$ = new tacco::IfStatementNode(std::unique_ptr<tacco::ExpressionNode>($3),
                                        std::unique_ptr<tacco::StatementNode>($5),
                                        std::unique_ptr<tacco::StatementNode>($7),
                                        MAKE_LOC(@$));
    }
    ;

while_statement
    : KW_WHILE LPAREN expression RPAREN statement {
        $$ = new tacco::WhileStatementNode(std::unique_ptr<tacco::ExpressionNode>($3),
                                           std::unique_ptr<tacco::StatementNode>($5),
                                           MAKE_LOC(@$));
    }
    ;

for_init
    : expression_statement { $$ = $1; }
    | variable_declaration { $$ = $1; }
    ;

for_statement
    : KW_FOR LPAREN for_init optional_expression SEMICOLON optional_expression RPAREN statement {
        $$ = new tacco::ForStatementNode(std::unique_ptr<tacco::StatementNode>($3),
                                         std::unique_ptr<tacco::ExpressionNode>($4),
                                         std::unique_ptr<tacco::ExpressionNode>($6),
                                         std::unique_ptr<tacco::StatementNode>($8),
                                         MAKE_LOC(@$));
    }
    ;

return_statement
    : KW_RETURN optional_expression SEMICOLON {
        $$ = new tacco::ReturnStatementNode(std::unique_ptr<tacco::ExpressionNode>($2), MAKE_LOC(@$));
    }
    ;

optional_expression
    : /* empty */ { $$ = nullptr; }
    | expression { $$ = $1; }
    ;

/* Expressions */
expression
    : assignment_expression { $$ = $1; }
    ;

assignment_expression
    : unary_expression EQUAL expression {
        $$ = new tacco::AssignmentNode(std::unique_ptr<tacco::ExpressionNode>($1),
                                       std::unique_ptr<tacco::ExpressionNode>($3),
                                       MAKE_LOC(@$));
    }
    | expression PLUS expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::ADD,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression MINUS expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::SUB,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression STAR expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::MUL,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression SLASH expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::DIV,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression PERCENT expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::MOD,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression EQUAL_EQUAL expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::EQ,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression BANG_EQUAL expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::NE,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression LESS expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::LT,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression GREATER expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::GT,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression LESS_EQUAL expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::LE,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression GREATER_EQUAL expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::GE,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression AND_AND expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::LOGICAL_AND,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | expression OR_OR expression {
        $$ = new tacco::BinaryExpressionNode(tacco::BinaryOp::LOGICAL_OR,
                                            std::unique_ptr<tacco::ExpressionNode>($1),
                                            std::unique_ptr<tacco::ExpressionNode>($3),
                                            MAKE_LOC(@$));
    }
    | unary_expression { $$ = $1; }
    ;

unary_expression
    : PLUS unary_expression %prec UPLUS {
        $$ = new tacco::UnaryExpressionNode(tacco::UnaryOp::PLUS,
                                           std::unique_ptr<tacco::ExpressionNode>($2),
                                           MAKE_LOC(@$));
    }
    | MINUS unary_expression %prec UMINUS {
        $$ = new tacco::UnaryExpressionNode(tacco::UnaryOp::MINUS,
                                           std::unique_ptr<tacco::ExpressionNode>($2),
                                           MAKE_LOC(@$));
    }
    | BANG unary_expression {
        $$ = new tacco::UnaryExpressionNode(tacco::UnaryOp::LOGICAL_NOT,
                                           std::unique_ptr<tacco::ExpressionNode>($2),
                                           MAKE_LOC(@$));
    }
    | STAR unary_expression %prec UNARY_DEREF {
        $$ = new tacco::DereferenceNode(std::unique_ptr<tacco::ExpressionNode>($2), MAKE_LOC(@$));
    }
    | AMPERSAND unary_expression %prec UNARY_ADDR {
        $$ = new tacco::AddressOfNode(std::unique_ptr<tacco::ExpressionNode>($2), MAKE_LOC(@$));
    }
    | postfix_expression { $$ = $1; }
    ;

argument_list
    : expression {
        $$ = new std::vector<std::unique_ptr<tacco::ExpressionNode>>();
        $$->push_back(std::unique_ptr<tacco::ExpressionNode>($1));
    }
    | argument_list COMMA expression {
        $$ = $1;
        $$->push_back(std::unique_ptr<tacco::ExpressionNode>($3));
    }
    ;

argument_list_opt
    : /* empty */ { $$ = new std::vector<std::unique_ptr<tacco::ExpressionNode>>(); }
    | argument_list { $$ = $1; }
    ;

postfix_expression
    : primary_expression { $$ = $1; }
    | postfix_expression LBRACKET expression RBRACKET {
        $$ = new tacco::ArrayAccessNode(std::unique_ptr<tacco::ExpressionNode>($1),
                                       std::unique_ptr<tacco::ExpressionNode>($3),
                                       MAKE_LOC(@$));
    }
    | IDENTIFIER LPAREN argument_list_opt RPAREN {
        std::vector<std::unique_ptr<tacco::ExpressionNode>> args;
        if ($3) {
            args = std::move(*$3);
            delete $3;
        }
        $$ = new tacco::FunctionCallNode(*$1, std::move(args), MAKE_LOC(@$));
        delete $1;
    }
    | postfix_expression DOT IDENTIFIER {
        $$ = new tacco::MemberAccessNode(std::unique_ptr<tacco::ExpressionNode>($1), *$3, MAKE_LOC(@$));
        delete $3;
    }
    ;

primary_expression
    : IDENTIFIER {
        $$ = new tacco::VariableAccessNode(*$1, MAKE_LOC(@$));
        delete $1;
    }
    | INTEGER_LITERAL {
        $$ = new tacco::IntegerLiteralNode($1, MAKE_LOC(@$));
    }
    | CHAR_LITERAL {
        $$ = new tacco::CharLiteralNode($1, MAKE_LOC(@$));
    }
    | STRING_LITERAL {
        $$ = new tacco::StringLiteralNode(*$1, MAKE_LOC(@$));
        delete $1;
    }
    | LPAREN expression RPAREN {
        $$ = $2;
    }
    ;

%%
