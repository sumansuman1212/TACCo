#ifndef TACCO_EXPRESSIONS_HPP
#define TACCO_EXPRESSIONS_HPP

#include "ast.hpp"
#include <string>
#include <vector>
#include <memory>

namespace tacco {

enum class BinaryOp {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    EQ,
    NE,
    LT,
    GT,
    LE,
    GE,
    LOGICAL_AND,
    LOGICAL_OR
};

const char* binary_op_to_string(BinaryOp op);

enum class UnaryOp {
    PLUS,
    MINUS,
    LOGICAL_NOT,
    DEREFERENCE,
    ADDRESS_OF
};

const char* unary_op_to_string(UnaryOp op);

// Integer literal
class IntegerLiteralNode : public ExpressionNode {
public:
    int64_t value;

    IntegerLiteralNode(int64_t val, SourceLocation loc)
        : ExpressionNode(std::move(loc)), value(val) {
        evaluated_type = Type::make_int();
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Character literal
class CharLiteralNode : public ExpressionNode {
public:
    char value;

    CharLiteralNode(char val, SourceLocation loc)
        : ExpressionNode(std::move(loc)), value(val) {
        evaluated_type = Type::make_char();
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// String literal
class StringLiteralNode : public ExpressionNode {
public:
    std::string value;

    StringLiteralNode(std::string val, SourceLocation loc)
        : ExpressionNode(std::move(loc)), value(std::move(val)) {
        evaluated_type = Type::make_pointer(Type::make_char());
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Variable reference
class VariableAccessNode : public ExpressionNode {
public:
    std::string name;

    VariableAccessNode(std::string n, SourceLocation loc)
        : ExpressionNode(std::move(loc)), name(std::move(n)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Binary expression
class BinaryExpressionNode : public ExpressionNode {
public:
    BinaryOp op;
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;

    BinaryExpressionNode(BinaryOp o, std::unique_ptr<ExpressionNode> l,
                         std::unique_ptr<ExpressionNode> r, SourceLocation loc)
        : ExpressionNode(std::move(loc)), op(o), left(std::move(l)), right(std::move(r)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Unary expression
class UnaryExpressionNode : public ExpressionNode {
public:
    UnaryOp op;
    std::unique_ptr<ExpressionNode> operand;

    UnaryExpressionNode(UnaryOp o, std::unique_ptr<ExpressionNode> opnd, SourceLocation loc)
        : ExpressionNode(std::move(loc)), op(o), operand(std::move(opnd)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Assignment expression: target = value
class AssignmentNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> target;
    std::unique_ptr<ExpressionNode> value;

    AssignmentNode(std::unique_ptr<ExpressionNode> tgt,
                   std::unique_ptr<ExpressionNode> val, SourceLocation loc)
        : ExpressionNode(std::move(loc)), target(std::move(tgt)), value(std::move(val)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Function call: callee(args...)
class FunctionCallNode : public ExpressionNode {
public:
    std::string callee_name;
    std::vector<std::unique_ptr<ExpressionNode>> args;

    FunctionCallNode(std::string name, std::vector<std::unique_ptr<ExpressionNode>> a, SourceLocation loc)
        : ExpressionNode(std::move(loc)), callee_name(std::move(name)), args(std::move(a)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Array subscript: array_expr[index_expr]
class ArrayAccessNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> array_expr;
    std::unique_ptr<ExpressionNode> index_expr;

    ArrayAccessNode(std::unique_ptr<ExpressionNode> arr,
                    std::unique_ptr<ExpressionNode> idx, SourceLocation loc)
        : ExpressionNode(std::move(loc)), array_expr(std::move(arr)), index_expr(std::move(idx)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Member access: object_expr.member_name
class MemberAccessNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> object_expr;
    std::string member_name;

    MemberAccessNode(std::unique_ptr<ExpressionNode> obj,
                     std::string mem, SourceLocation loc)
        : ExpressionNode(std::move(loc)), object_expr(std::move(obj)), member_name(std::move(mem)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Address-of: &operand
class AddressOfNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> operand;

    AddressOfNode(std::unique_ptr<ExpressionNode> opnd, SourceLocation loc)
        : ExpressionNode(std::move(loc)), operand(std::move(opnd)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Dereference: *operand
class DereferenceNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> operand;

    DereferenceNode(std::unique_ptr<ExpressionNode> opnd, SourceLocation loc)
        : ExpressionNode(std::move(loc)), operand(std::move(opnd)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

} // namespace tacco

#endif // TACCO_EXPRESSIONS_HPP
