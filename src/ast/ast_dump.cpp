#include "ast.hpp"
#include "expressions.hpp"
#include "statements.hpp"
#include "declarations.hpp"
#include <iomanip>

namespace tacco {

static void print_indent(std::ostream& os, int indent) {
    for (int i = 0; i < indent; ++i) {
        os << "  ";
    }
}

const char* binary_op_to_string(BinaryOp op) {
    switch (op) {
        case BinaryOp::ADD:         return "+";
        case BinaryOp::SUB:         return "-";
        case BinaryOp::MUL:         return "*";
        case BinaryOp::DIV:         return "/";
        case BinaryOp::MOD:         return "%";
        case BinaryOp::EQ:          return "==";
        case BinaryOp::NE:          return "!=";
        case BinaryOp::LT:          return "<";
        case BinaryOp::GT:          return ">";
        case BinaryOp::LE:          return "<=";
        case BinaryOp::GE:          return ">=";
        case BinaryOp::LOGICAL_AND: return "&&";
        case BinaryOp::LOGICAL_OR:  return "||";
    }
    return "unknown_binary_op";
}

const char* unary_op_to_string(UnaryOp op) {
    switch (op) {
        case UnaryOp::PLUS:         return "+";
        case UnaryOp::MINUS:        return "-";
        case UnaryOp::LOGICAL_NOT:  return "!";
        case UnaryOp::DEREFERENCE:  return "*";
        case UnaryOp::ADDRESS_OF:   return "&";
    }
    return "unknown_unary_op";
}

// Expressions dump
void IntegerLiteralNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "IntegerLiteral: " << value << " [" << location.to_string() << "]\n";
}

void CharLiteralNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "CharLiteral: '" << value << "' (0x" << std::hex << (int)(unsigned char)value << std::dec << ") [" << location.to_string() << "]\n";
}

void StringLiteralNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "StringLiteral: \"" << value << "\" [" << location.to_string() << "]\n";
}

void VariableAccessNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "VariableAccess: " << name << " [" << location.to_string() << "]\n";
}

void BinaryExpressionNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "BinaryExpression (" << binary_op_to_string(op) << ") [" << location.to_string() << "]\n";
    if (left) left->dump(os, indent + 1);
    if (right) right->dump(os, indent + 1);
}

void UnaryExpressionNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "UnaryExpression (" << unary_op_to_string(op) << ") [" << location.to_string() << "]\n";
    if (operand) operand->dump(os, indent + 1);
}

void AssignmentNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "Assignment [" << location.to_string() << "]\n";
    print_indent(os, indent + 1);
    os << "Target:\n";
    if (target) target->dump(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Value:\n";
    if (value) value->dump(os, indent + 2);
}

void FunctionCallNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "FunctionCall: " << callee_name << " (" << args.size() << " args) [" << location.to_string() << "]\n";
    for (const auto& arg : args) {
        if (arg) arg->dump(os, indent + 1);
    }
}

void ArrayAccessNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "ArrayAccess [" << location.to_string() << "]\n";
    print_indent(os, indent + 1);
    os << "Base:\n";
    if (array_expr) array_expr->dump(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Index:\n";
    if (index_expr) index_expr->dump(os, indent + 2);
}

void MemberAccessNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "MemberAccess: ." << member_name << " [" << location.to_string() << "]\n";
    if (object_expr) object_expr->dump(os, indent + 1);
}

void AddressOfNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "AddressOf (&) [" << location.to_string() << "]\n";
    if (operand) operand->dump(os, indent + 1);
}

void DereferenceNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "Dereference (*) [" << location.to_string() << "]\n";
    if (operand) operand->dump(os, indent + 1);
}

// Statements dump
void ExpressionStatementNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "ExpressionStatement [" << location.to_string() << "]\n";
    if (expression) expression->dump(os, indent + 1);
}

void CompoundStatementNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "CompoundStatement (" << statements.size() << " statements) [" << location.to_string() << "]\n";
    for (const auto& stmt : statements) {
        if (stmt) stmt->dump(os, indent + 1);
    }
}

void IfStatementNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "IfStatement [" << location.to_string() << "]\n";
    print_indent(os, indent + 1);
    os << "Condition:\n";
    if (condition) condition->dump(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Then:\n";
    if (then_branch) then_branch->dump(os, indent + 2);
    if (else_branch) {
        print_indent(os, indent + 1);
        os << "Else:\n";
        else_branch->dump(os, indent + 2);
    }
}

void WhileStatementNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "WhileStatement [" << location.to_string() << "]\n";
    print_indent(os, indent + 1);
    os << "Condition:\n";
    if (condition) condition->dump(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Body:\n";
    if (body) body->dump(os, indent + 2);
}

void ForStatementNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "ForStatement [" << location.to_string() << "]\n";
    if (init) {
        print_indent(os, indent + 1);
        os << "Init:\n";
        init->dump(os, indent + 2);
    }
    if (condition) {
        print_indent(os, indent + 1);
        os << "Condition:\n";
        condition->dump(os, indent + 2);
    }
    if (update) {
        print_indent(os, indent + 1);
        os << "Update:\n";
        update->dump(os, indent + 2);
    }
    if (body) {
        print_indent(os, indent + 1);
        os << "Body:\n";
        body->dump(os, indent + 2);
    }
}

void ReturnStatementNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "ReturnStatement [" << location.to_string() << "]\n";
    if (expression) expression->dump(os, indent + 1);
}

// Declarations dump
void VariableDeclarationNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "VariableDeclaration: " << (type ? type->to_string() : "unknown") << " " << name;
    if (is_array) {
        os << "[" << array_size << "]";
    }
    os << " [" << location.to_string() << "]\n";
    if (initializer) {
        print_indent(os, indent + 1);
        os << "Initializer:\n";
        initializer->dump(os, indent + 2);
    }
}

void ParameterNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "Parameter: " << (type ? type->to_string() : "unknown") << " " << name << " [" << location.to_string() << "]\n";
}

void FunctionDeclarationNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "FunctionDeclaration: " << (return_type ? return_type->to_string() : "void")
       << " " << name << " (" << params.size() << " params) [" << location.to_string() << "]\n";
    for (const auto& p : params) {
        if (p) p->dump(os, indent + 1);
    }
}

void FunctionDefinitionNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "FunctionDefinition: " << (return_type ? return_type->to_string() : "void")
       << " " << name << " (" << params.size() << " params) [" << location.to_string() << "]\n";
    for (const auto& p : params) {
        if (p) p->dump(os, indent + 1);
    }
    if (body) {
        print_indent(os, indent + 1);
        os << "Body:\n";
        body->dump(os, indent + 2);
    }
}

void FieldDeclarationNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "FieldDeclaration: " << (type ? type->to_string() : "unknown") << " " << name;
    if (is_array) {
        os << "[" << array_size << "]";
    }
    os << " [" << location.to_string() << "]\n";
}

void ClassDeclarationNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "ClassDeclaration: class " << name << " (" << fields.size() << " fields) [" << location.to_string() << "]\n";
    for (const auto& f : fields) {
        if (f) f->dump(os, indent + 1);
    }
}

void ProgramNode::dump(std::ostream& os, int indent) const {
    print_indent(os, indent);
    os << "Program (" << declarations.size() << " top-level declarations) [" << location.to_string() << "]\n";
    for (const auto& decl : declarations) {
        if (decl) decl->dump(os, indent + 1);
    }
}

} // namespace tacco
