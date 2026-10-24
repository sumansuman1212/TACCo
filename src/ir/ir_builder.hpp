#ifndef TACCO_IR_BUILDER_HPP
#define TACCO_IR_BUILDER_HPP

#include "ir.hpp"
#include "../ast/ast.hpp"
#include "../ast/expressions.hpp"
#include "../ast/statements.hpp"
#include "../ast/declarations.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace tacco {

class IRBuilder : public ASTVisitor {
public:
    IRBuilder();
    ~IRBuilder() override = default;

    std::unique_ptr<IRProgram> build(ProgramNode& program);

    // Expressions
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

    // Statements
    void visit(ExpressionStatementNode& node) override;
    void visit(CompoundStatementNode& node) override;
    void visit(IfStatementNode& node) override;
    void visit(WhileStatementNode& node) override;
    void visit(ForStatementNode& node) override;
    void visit(ReturnStatementNode& node) override;

    // Declarations
    void visit(VariableDeclarationNode& node) override;
    void visit(ParameterNode& node) override;
    void visit(FunctionDeclarationNode& node) override;
    void visit(FunctionDefinitionNode& node) override;
    void visit(FieldDeclarationNode& node) override;
    void visit(ClassDeclarationNode& node) override;
    void visit(ProgramNode& node) override;

    Operand get_last_operand() const { return last_operand_; }

private:
    std::string new_temp();
    std::string new_label();

    Operand evaluate_expr(ExpressionNode& expr);
    int calculate_type_size(const TypePtr& type);

    std::unique_ptr<IRProgram> program_;
    IRFunction* current_fn_ = nullptr;
    Operand last_operand_;

    int temp_counter_ = 0;
    int label_counter_ = 0;
    int str_counter_ = 0;
};

} // namespace tacco

#endif // TACCO_IR_BUILDER_HPP
