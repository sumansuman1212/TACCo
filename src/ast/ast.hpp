#ifndef TACCO_AST_HPP
#define TACCO_AST_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "../common/location.hpp"
#include "types.hpp"

namespace tacco {

// Forward declarations
class ASTVisitor;

class IntegerLiteralNode;
class CharLiteralNode;
class StringLiteralNode;
class VariableAccessNode;
class BinaryExpressionNode;
class UnaryExpressionNode;
class AssignmentNode;
class FunctionCallNode;
class ArrayAccessNode;
class MemberAccessNode;
class AddressOfNode;
class DereferenceNode;

class ExpressionStatementNode;
class CompoundStatementNode;
class IfStatementNode;
class WhileStatementNode;
class ForStatementNode;
class ReturnStatementNode;

class VariableDeclarationNode;
class ParameterNode;
class FunctionDeclarationNode;
class FunctionDefinitionNode;
class FieldDeclarationNode;
class ClassDeclarationNode;
class ProgramNode;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Expressions
    virtual void visit(IntegerLiteralNode& node) = 0;
    virtual void visit(CharLiteralNode& node) = 0;
    virtual void visit(StringLiteralNode& node) = 0;
    virtual void visit(VariableAccessNode& node) = 0;
    virtual void visit(BinaryExpressionNode& node) = 0;
    virtual void visit(UnaryExpressionNode& node) = 0;
    virtual void visit(AssignmentNode& node) = 0;
    virtual void visit(FunctionCallNode& node) = 0;
    virtual void visit(ArrayAccessNode& node) = 0;
    virtual void visit(MemberAccessNode& node) = 0;
    virtual void visit(AddressOfNode& node) = 0;
    virtual void visit(DereferenceNode& node) = 0;

    // Statements
    virtual void visit(ExpressionStatementNode& node) = 0;
    virtual void visit(CompoundStatementNode& node) = 0;
    virtual void visit(IfStatementNode& node) = 0;
    virtual void visit(WhileStatementNode& node) = 0;
    virtual void visit(ForStatementNode& node) = 0;
    virtual void visit(ReturnStatementNode& node) = 0;

    // Declarations
    virtual void visit(VariableDeclarationNode& node) = 0;
    virtual void visit(ParameterNode& node) = 0;
    virtual void visit(FunctionDeclarationNode& node) = 0;
    virtual void visit(FunctionDefinitionNode& node) = 0;
    virtual void visit(FieldDeclarationNode& node) = 0;
    virtual void visit(ClassDeclarationNode& node) = 0;
    virtual void visit(ProgramNode& node) = 0;
};

class Scope;

class ASTNode {
public:
    SourceLocation location;
    TypePtr evaluated_type = nullptr; // Inferred or checked type
    std::shared_ptr<Scope> scope = nullptr; // Lexical scope attached to this node (if any)

    explicit ASTNode(SourceLocation loc) : location(std::move(loc)) {}
    virtual ~ASTNode() = default;

    virtual void accept(ASTVisitor& visitor) = 0;
    virtual void dump(std::ostream& os, int indent = 0) const = 0;

    std::string dump_string(int indent = 0) const;
};

class ExpressionNode : public ASTNode {
public:
    explicit ExpressionNode(SourceLocation loc) : ASTNode(std::move(loc)) {}
};

class StatementNode : public ASTNode {
public:
    explicit StatementNode(SourceLocation loc) : ASTNode(std::move(loc)) {}
};

class DeclarationNode : public StatementNode {
public:
    explicit DeclarationNode(SourceLocation loc) : StatementNode(std::move(loc)) {}
};

} // namespace tacco

#endif // TACCO_AST_HPP
