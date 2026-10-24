#ifndef TACCO_STATEMENTS_HPP
#define TACCO_STATEMENTS_HPP

#include "ast.hpp"
#include "expressions.hpp"
#include <vector>
#include <memory>

namespace tacco {

// Expression statement: expr;
class ExpressionStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> expression;

    ExpressionStatementNode(std::unique_ptr<ExpressionNode> expr, SourceLocation loc)
        : StatementNode(std::move(loc)), expression(std::move(expr)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Compound statement: { stmt1; stmt2; ... }
class CompoundStatementNode : public StatementNode {
public:
    std::vector<std::unique_ptr<StatementNode>> statements;

    explicit CompoundStatementNode(SourceLocation loc)
        : StatementNode(std::move(loc)) {}

    CompoundStatementNode(std::vector<std::unique_ptr<StatementNode>> stmts, SourceLocation loc)
        : StatementNode(std::move(loc)), statements(std::move(stmts)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// If statement: if (condition) then_branch else else_branch
class IfStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<StatementNode> then_branch;
    std::unique_ptr<StatementNode> else_branch; // May be nullptr

    IfStatementNode(std::unique_ptr<ExpressionNode> cond,
                    std::unique_ptr<StatementNode> then_b,
                    std::unique_ptr<StatementNode> else_b,
                    SourceLocation loc)
        : StatementNode(std::move(loc)),
          condition(std::move(cond)),
          then_branch(std::move(then_b)),
          else_branch(std::move(else_b)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// While loop: while (condition) body
class WhileStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<StatementNode> body;

    WhileStatementNode(std::unique_ptr<ExpressionNode> cond,
                       std::unique_ptr<StatementNode> b,
                       SourceLocation loc)
        : StatementNode(std::move(loc)), condition(std::move(cond)), body(std::move(b)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// For loop: for (init; condition; update) body
class ForStatementNode : public StatementNode {
public:
    std::unique_ptr<StatementNode> init;
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<ExpressionNode> update;
    std::unique_ptr<StatementNode> body;

    ForStatementNode(std::unique_ptr<StatementNode> i,
                     std::unique_ptr<ExpressionNode> cond,
                     std::unique_ptr<ExpressionNode> upd,
                     std::unique_ptr<StatementNode> b,
                     SourceLocation loc)
        : StatementNode(std::move(loc)),
          init(std::move(i)),
          condition(std::move(cond)),
          update(std::move(upd)),
          body(std::move(b)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Return statement: return expr?;
class ReturnStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> expression; // May be nullptr for void return

    ReturnStatementNode(std::unique_ptr<ExpressionNode> expr, SourceLocation loc)
        : StatementNode(std::move(loc)), expression(std::move(expr)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

} // namespace tacco

#endif // TACCO_STATEMENTS_HPP
