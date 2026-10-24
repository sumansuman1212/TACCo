#ifndef TACCO_TYPE_CHECKER_HPP
#define TACCO_TYPE_CHECKER_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "symbol_table.hpp"
#include "../ast/ast.hpp"
#include "../ast/expressions.hpp"
#include "../ast/statements.hpp"
#include "../ast/declarations.hpp"

namespace tacco {

class TypeChecker : public ASTVisitor {
public:
    TypeChecker();
    explicit TypeChecker(SymbolTable& symtab);
    ~TypeChecker() override = default;

    bool check(ProgramNode& program);

    bool has_errors() const { return !error_messages_.empty(); }
    const std::vector<std::string>& get_error_messages() const { return error_messages_; }
    void record_error(const std::string& msg, const SourceLocation& loc);

    SymbolTable& get_symbol_table() { return symbol_table_; }

    // Helpers
    static bool is_lvalue(const ExpressionNode& expr);
    static bool is_type_compatible(const TypePtr& target, const TypePtr& source);
    static bool is_boolean_compatible(const TypePtr& type);

    // AST Visitor methods
    void visit(IntegerLiteralNode& node) override;
    void visit(CharLiteralNode& node) override;
    void visit(StringLiteralNode& node) override;
    void visit(VariableAccessNode& node) override;
    void visit(BinaryExpressionNode& node) override;
    void visit(UnaryExpressionNode& node) override;
    void visit(AssignmentNode& node) override;
    void visit(FunctionCallNode& node) override;
    void visit(ArrayAccessNode& node) override;
    void visit(MemberAccessNode& node) override;
    void visit(AddressOfNode& node) override;
    void visit(DereferenceNode& node) override;

    void visit(ExpressionStatementNode& node) override;
    void visit(CompoundStatementNode& node) override;
    void visit(IfStatementNode& node) override;
    void visit(WhileStatementNode& node) override;
    void visit(ForStatementNode& node) override;
    void visit(ReturnStatementNode& node) override;

    void visit(VariableDeclarationNode& node) override;
    void visit(ParameterNode& node) override;
    void visit(FunctionDeclarationNode& node) override;
    void visit(FunctionDefinitionNode& node) override;
    void visit(FieldDeclarationNode& node) override;
    void visit(ClassDeclarationNode& node) override;
    void visit(ProgramNode& node) override;

private:
    SymbolTable symbol_table_;
    TypePtr current_function_return_type_ = nullptr;
    std::vector<std::string> error_messages_;
    ScopePtr current_scope_;
};

} // namespace tacco

#endif // TACCO_TYPE_CHECKER_HPP
