#include "type_checker.hpp"
#include <sstream>

namespace tacco {

TypeChecker::TypeChecker() = default;

TypeChecker::TypeChecker(SymbolTable& symtab) : symbol_table_(symtab) {}

void TypeChecker::record_error(const std::string& msg, const SourceLocation& loc) {
    std::string formatted = "semantic error: " + msg + " at " + loc.to_string();
    error_messages_.push_back(formatted);
}

bool TypeChecker::is_lvalue(const ExpressionNode& expr) {
    if (dynamic_cast<const VariableAccessNode*>(&expr)) return true;
    if (dynamic_cast<const ArrayAccessNode*>(&expr)) return true;
    if (dynamic_cast<const DereferenceNode*>(&expr)) return true;
    if (dynamic_cast<const MemberAccessNode*>(&expr)) return true;
    return false;
}

bool TypeChecker::is_type_compatible(const TypePtr& target, const TypePtr& source) {
    if (!target || !source) return false;

    // Direct equality
    if (target->equals(*source)) return true;

    // Integral conversions (int and char)
    if (target->is_integral() && source->is_integral()) return true;

    // Array to pointer decay: T* target = T[N] source
    if (target->is_pointer() && source->is_array()) {
        return target->base_type && source->base_type && target->base_type->equals(*source->base_type);
    }

    // void* compatibility
    if (target->is_pointer() && source->is_pointer()) {
        if (target->base_type->is_void() || source->base_type->is_void()) return true;
    }

    return false;
}

bool TypeChecker::is_boolean_compatible(const TypePtr& type) {
    if (!type) return false;
    return type->is_integral() || type->is_pointer();
}

bool TypeChecker::check(ProgramNode& program) {
    error_messages_.clear();

    // First build the symbol table scopes
    if (!symbol_table_.build(program)) {
        for (const auto& err : symbol_table_.get_error_messages()) {
            error_messages_.push_back(err);
        }
        return false;
    }

    current_scope_ = symbol_table_.get_global_scope();
    program.accept(*this);

    return !has_errors();
}

// Expression visitors
void TypeChecker::visit(IntegerLiteralNode& node) {
    node.evaluated_type = Type::make_int();
}

void TypeChecker::visit(CharLiteralNode& node) {
    node.evaluated_type = Type::make_char();
}

void TypeChecker::visit(StringLiteralNode& node) {
    node.evaluated_type = Type::make_pointer(Type::make_char());
}

void TypeChecker::visit(VariableAccessNode& node) {
    auto sym = current_scope_->lookup(node.name);
    if (!sym) {
        record_error("use of undeclared identifier '" + node.name + "'", node.location);
        node.evaluated_type = Type::make_int(); // Fallback
        return;
    }
    node.evaluated_type = sym->type;
}

void TypeChecker::visit(BinaryExpressionNode& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);

    auto left_type = node.left ? node.left->evaluated_type : nullptr;
    auto right_type = node.right ? node.right->evaluated_type : nullptr;

    if (!left_type || !right_type) {
        node.evaluated_type = Type::make_int();
        return;
    }

    switch (node.op) {
        case BinaryOp::ADD:
            if (left_type->is_integral() && right_type->is_integral()) {
                node.evaluated_type = Type::make_int();
            } else if (left_type->is_pointer() && right_type->is_integral()) {
                node.evaluated_type = left_type;
            } else if (left_type->is_integral() && right_type->is_pointer()) {
                node.evaluated_type = right_type;
            } else {
                record_error("invalid operands to binary '+' (" + left_type->to_string() + " and " + right_type->to_string() + ")", node.location);
                node.evaluated_type = Type::make_int();
            }
            break;

        case BinaryOp::SUB:
            if (left_type->is_integral() && right_type->is_integral()) {
                node.evaluated_type = Type::make_int();
            } else if (left_type->is_pointer() && right_type->is_integral()) {
                node.evaluated_type = left_type;
            } else if (left_type->is_pointer() && right_type->is_pointer() && left_type->equals(*right_type)) {
                node.evaluated_type = Type::make_int();
            } else {
                record_error("invalid operands to binary '-' (" + left_type->to_string() + " and " + right_type->to_string() + ")", node.location);
                node.evaluated_type = Type::make_int();
            }
            break;

        case BinaryOp::MUL:
        case BinaryOp::DIV:
        case BinaryOp::MOD:
            if (left_type->is_integral() && right_type->is_integral()) {
                node.evaluated_type = Type::make_int();
            } else {
                record_error("invalid operands to binary arithmetic operator (" + left_type->to_string() + " and " + right_type->to_string() + ")", node.location);
                node.evaluated_type = Type::make_int();
            }
            break;

        case BinaryOp::EQ:
        case BinaryOp::NE:
        case BinaryOp::LT:
        case BinaryOp::GT:
        case BinaryOp::LE:
        case BinaryOp::GE:
            if ((left_type->is_integral() && right_type->is_integral()) ||
                (left_type->is_pointer() && right_type->is_pointer())) {
                node.evaluated_type = Type::make_int();
            } else {
                record_error("comparison between incompatible types (" + left_type->to_string() + " and " + right_type->to_string() + ")", node.location);
                node.evaluated_type = Type::make_int();
            }
            break;

        case BinaryOp::LOGICAL_AND:
        case BinaryOp::LOGICAL_OR:
            if (is_boolean_compatible(left_type) && is_boolean_compatible(right_type)) {
                node.evaluated_type = Type::make_int();
            } else {
                record_error("invalid operands to logical operator (" + left_type->to_string() + " and " + right_type->to_string() + ")", node.location);
                node.evaluated_type = Type::make_int();
            }
            break;
    }
}

void TypeChecker::visit(UnaryExpressionNode& node) {
    if (node.operand) node.operand->accept(*this);
    auto opnd_type = node.operand ? node.operand->evaluated_type : nullptr;

    if (!opnd_type) {
        node.evaluated_type = Type::make_int();
        return;
    }

    switch (node.op) {
        case UnaryOp::PLUS:
        case UnaryOp::MINUS:
            if (!opnd_type->is_integral()) {
                record_error("invalid operand to unary arithmetic operator (" + opnd_type->to_string() + ")", node.location);
            }
            node.evaluated_type = Type::make_int();
            break;

        case UnaryOp::LOGICAL_NOT:
            if (!is_boolean_compatible(opnd_type)) {
                record_error("invalid operand to unary '!' (" + opnd_type->to_string() + ")", node.location);
            }
            node.evaluated_type = Type::make_int();
            break;

        case UnaryOp::DEREFERENCE:
        case UnaryOp::ADDRESS_OF:
            // Handled in dedicated classes DereferenceNode and AddressOfNode
            break;
    }
}

void TypeChecker::visit(AddressOfNode& node) {
    if (!node.operand) {
        node.evaluated_type = Type::make_pointer(Type::make_void());
        return;
    }

    if (!is_lvalue(*node.operand)) {
        record_error("cannot take address of an rvalue", node.location);
    }

    node.operand->accept(*this);
    auto opnd_type = node.operand->evaluated_type;
    if (opnd_type) {
        node.evaluated_type = Type::make_pointer(opnd_type);
    } else {
        node.evaluated_type = Type::make_pointer(Type::make_void());
    }
}

void TypeChecker::visit(DereferenceNode& node) {
    if (!node.operand) {
        node.evaluated_type = Type::make_int();
        return;
    }

    node.operand->accept(*this);
    auto opnd_type = node.operand->evaluated_type;

    if (!opnd_type || !opnd_type->is_pointer()) {
        record_error("cannot dereference non-pointer type '" + (opnd_type ? opnd_type->to_string() : "unknown") + "'", node.location);
        node.evaluated_type = Type::make_int();
        return;
    }

    node.evaluated_type = opnd_type->base_type;
}

void TypeChecker::visit(AssignmentNode& node) {
    if (!node.target || !node.value) {
        node.evaluated_type = Type::make_int();
        return;
    }

    if (!is_lvalue(*node.target)) {
        record_error("assignment target is not an assignable lvalue", node.location);
    }

    node.target->accept(*this);
    node.value->accept(*this);

    auto target_type = node.target->evaluated_type;
    auto value_type = node.value->evaluated_type;

    if (target_type && value_type) {
        if (!is_type_compatible(target_type, value_type)) {
            record_error("type mismatch in assignment: cannot assign '" + value_type->to_string() + "' to '" + target_type->to_string() + "'", node.location);
        }
        node.evaluated_type = target_type;
    } else {
        node.evaluated_type = Type::make_int();
    }
}

void TypeChecker::visit(FunctionCallNode& node) {
    auto sym = current_scope_->lookup(node.callee_name);
    if (!sym || !sym->is_function()) {
        record_error("call to undeclared function '" + node.callee_name + "'", node.location);
        node.evaluated_type = Type::make_int();
        return;
    }

    if (node.args.size() != sym->parameters.size()) {
        record_error("function '" + node.callee_name + "' expects " +
                     std::to_string(sym->parameters.size()) + " arguments, but got " +
                     std::to_string(node.args.size()), node.location);
    }

    size_t check_count = std::min(node.args.size(), sym->parameters.size());
    for (size_t i = 0; i < check_count; ++i) {
        if (node.args[i]) {
            node.args[i]->accept(*this);
            auto arg_type = node.args[i]->evaluated_type;
            auto param_type = sym->parameters[i]->type;
            if (!is_type_compatible(param_type, arg_type)) {
                record_error("type mismatch for argument " + std::to_string(i + 1) + " of function '" +
                             node.callee_name + "': expected '" + (param_type ? param_type->to_string() : "unknown") +
                             "', got '" + (arg_type ? arg_type->to_string() : "unknown") + "'", node.args[i]->location);
            }
        }
    }

    node.evaluated_type = sym->type->base_type; // Return type
}

void TypeChecker::visit(ArrayAccessNode& node) {
    if (!node.array_expr || !node.index_expr) {
        node.evaluated_type = Type::make_int();
        return;
    }

    node.array_expr->accept(*this);
    node.index_expr->accept(*this);

    auto base_type = node.array_expr->evaluated_type;
    auto index_type = node.index_expr->evaluated_type;

    if (!base_type || (!base_type->is_array() && !base_type->is_pointer())) {
        record_error("subscripted value is not an array or pointer ('" + (base_type ? base_type->to_string() : "unknown") + "')", node.location);
        node.evaluated_type = Type::make_int();
        return;
    }

    if (!index_type || !index_type->is_integral()) {
        record_error("array subscript is not an integer ('" + (index_type ? index_type->to_string() : "unknown") + "')", node.location);
    }

    node.evaluated_type = base_type->base_type;
}

void TypeChecker::visit(MemberAccessNode& node) {
    if (!node.object_expr) {
        node.evaluated_type = Type::make_int();
        return;
    }

    node.object_expr->accept(*this);
    auto obj_type = node.object_expr->evaluated_type;

    if (!obj_type || !obj_type->is_class()) {
        record_error("request for member '" + node.member_name + "' in something not a class ('" +
                     (obj_type ? obj_type->to_string() : "unknown") + "')", node.location);
        node.evaluated_type = Type::make_int();
        return;
    }

    auto class_sym = symbol_table_.get_global_scope()->lookup(obj_type->name);
    if (!class_sym || !class_sym->is_class()) {
        record_error("unknown class '" + obj_type->name + "'", node.location);
        node.evaluated_type = Type::make_int();
        return;
    }

    auto field_sym = class_sym->get_field(node.member_name);
    if (!field_sym) {
        record_error("class '" + obj_type->name + "' has no member named '" + node.member_name + "'", node.location);
        node.evaluated_type = Type::make_int();
        return;
    }

    node.evaluated_type = field_sym->type;
}

// Statement visitors
void TypeChecker::visit(ExpressionStatementNode& node) {
    if (node.expression) node.expression->accept(*this);
}

void TypeChecker::visit(CompoundStatementNode& node) {
    auto prev_scope = current_scope_;
    if (node.scope) current_scope_ = node.scope;
    for (auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
    current_scope_ = prev_scope;
}

void TypeChecker::visit(IfStatementNode& node) {
    if (node.condition) {
        node.condition->accept(*this);
        if (!is_boolean_compatible(node.condition->evaluated_type)) {
            record_error("if condition is not boolean-compatible", node.location);
        }
    }
    if (node.then_branch) node.then_branch->accept(*this);
    if (node.else_branch) node.else_branch->accept(*this);
}

void TypeChecker::visit(WhileStatementNode& node) {
    if (node.condition) {
        node.condition->accept(*this);
        if (!is_boolean_compatible(node.condition->evaluated_type)) {
            record_error("while condition is not boolean-compatible", node.location);
        }
    }
    if (node.body) node.body->accept(*this);
}

void TypeChecker::visit(ForStatementNode& node) {
    auto prev_scope = current_scope_;
    if (node.scope) current_scope_ = node.scope;

    if (node.init) node.init->accept(*this);
    if (node.condition) {
        node.condition->accept(*this);
        if (!is_boolean_compatible(node.condition->evaluated_type)) {
            record_error("for loop condition is not boolean-compatible", node.location);
        }
    }
    if (node.update) node.update->accept(*this);
    if (node.body) node.body->accept(*this);

    current_scope_ = prev_scope;
}

void TypeChecker::visit(ReturnStatementNode& node) {
    if (!current_function_return_type_) {
        record_error("return statement outside function", node.location);
        return;
    }

    if (node.expression) {
        node.expression->accept(*this);
        if (current_function_return_type_->is_void()) {
            record_error("return with a value in function returning void", node.location);
        } else if (!is_type_compatible(current_function_return_type_, node.expression->evaluated_type)) {
            record_error("type mismatch in return statement: expected '" +
                         current_function_return_type_->to_string() + "', got '" +
                         (node.expression->evaluated_type ? node.expression->evaluated_type->to_string() : "unknown") + "'",
                         node.location);
        }
    } else {
        if (!current_function_return_type_->is_void()) {
            record_error("return without a value in non-void function returning '" +
                         current_function_return_type_->to_string() + "'", node.location);
        }
    }
}

// Declaration visitors
void TypeChecker::visit(VariableDeclarationNode& node) {
    if (node.initializer) {
        node.initializer->accept(*this);
        if (!is_type_compatible(node.type, node.initializer->evaluated_type)) {
            record_error("type mismatch in variable initialization: cannot initialize '" +
                         (node.type ? node.type->to_string() : "unknown") + "' with '" +
                         (node.initializer->evaluated_type ? node.initializer->evaluated_type->to_string() : "unknown") + "'",
                         node.location);
        }
    }
}

void TypeChecker::visit(ParameterNode&) {}
void TypeChecker::visit(FunctionDeclarationNode&) {}

void TypeChecker::visit(FunctionDefinitionNode& node) {
    current_function_return_type_ = node.return_type;
    auto prev_scope = current_scope_;
    if (node.scope) current_scope_ = node.scope;

    for (auto& p : node.params) {
        if (p) p->accept(*this);
    }
    if (node.body) {
        for (auto& stmt : node.body->statements) {
            if (stmt) stmt->accept(*this);
        }
    }

    current_scope_ = prev_scope;
    current_function_return_type_ = nullptr;
}

void TypeChecker::visit(FieldDeclarationNode&) {}
void TypeChecker::visit(ClassDeclarationNode&) {}

void TypeChecker::visit(ProgramNode& node) {
    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }
}

} // namespace tacco
