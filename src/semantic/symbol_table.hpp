#ifndef TACCO_SYMBOL_TABLE_HPP
#define TACCO_SYMBOL_TABLE_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "symbol.hpp"
#include "scope.hpp"
#include "../ast/ast.hpp"
#include "../ast/declarations.hpp"
#include "../ast/statements.hpp"

namespace tacco {

class SymbolTable : public ASTVisitor {
public:
    SymbolTable();
    ~SymbolTable() override = default;

    void enter_scope(ScopeKind kind, const std::string& name = "");
    void exit_scope();

    ScopePtr get_global_scope() const { return global_scope_; }
    ScopePtr get_current_scope() const { return current_scope_; }

    bool insert(SymbolPtr symbol);
    SymbolPtr lookup(const std::string& name) const;
    SymbolPtr lookup_current(const std::string& name) const;

    void record_error(const std::string& msg, const SourceLocation& loc);
    bool has_errors() const { return !error_messages_.empty(); }
    const std::vector<std::string>& get_error_messages() const { return error_messages_; }

    // Build symbol tables from AST
    bool build(ProgramNode& program);

    // AST Visitor methods for collecting declarations
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

    void dump(std::ostream& os) const;

private:
    ScopePtr global_scope_;
    ScopePtr current_scope_;
    std::vector<std::string> error_messages_;
};

} // namespace tacco

#endif // TACCO_SYMBOL_TABLE_HPP
