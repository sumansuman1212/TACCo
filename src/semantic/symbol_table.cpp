#include "symbol_table.hpp"
#include <iostream>

namespace tacco {

SymbolTable::SymbolTable() {
    global_scope_ = std::make_shared<Scope>(ScopeKind::GLOBAL, nullptr, "global");
    current_scope_ = global_scope_;
}

void SymbolTable::enter_scope(ScopeKind kind, const std::string& name) {
    current_scope_ = current_scope_->add_child(kind, name);
}

void SymbolTable::exit_scope() {
    if (current_scope_->parent) {
        current_scope_ = current_scope_->parent;
    }
}

bool SymbolTable::insert(SymbolPtr symbol) {
    if (!symbol) return false;
    if (!current_scope_->insert(symbol)) {
        record_error("redefinition of '" + symbol->name + "'", symbol->location);
        return false;
    }
    return true;
}

SymbolPtr SymbolTable::lookup(const std::string& name) const {
    return current_scope_->lookup(name);
}

SymbolPtr SymbolTable::lookup_current(const std::string& name) const {
    return current_scope_->lookup_current(name);
}

void SymbolTable::record_error(const std::string& msg, const SourceLocation& loc) {
    std::string formatted = "scope error: " + msg + " at " + loc.to_string();
    error_messages_.push_back(formatted);
}

bool SymbolTable::build(ProgramNode& program) {
    error_messages_.clear();
    global_scope_ = std::make_shared<Scope>(ScopeKind::GLOBAL, nullptr, "global");
    current_scope_ = global_scope_;

    program.accept(*this);
    return !has_errors();
}

void SymbolTable::dump(std::ostream& os) const {
    if (global_scope_) {
        global_scope_->dump(os);
    }
}

// Expression visitors (no new scopes / symbols declared in primary expressions)
void SymbolTable::visit(IntegerLiteralNode&) {}
void SymbolTable::visit(CharLiteralNode&) {}
void SymbolTable::visit(StringLiteralNode&) {}
void SymbolTable::visit(VariableAccessNode&) {}
void SymbolTable::visit(BinaryExpressionNode& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
}
void SymbolTable::visit(UnaryExpressionNode& node) {
    if (node.operand) node.operand->accept(*this);
}
void SymbolTable::visit(AssignmentNode& node) {
    if (node.target) node.target->accept(*this);
    if (node.value) node.value->accept(*this);
}
void SymbolTable::visit(FunctionCallNode& node) {
    for (auto& arg : node.args) {
        if (arg) arg->accept(*this);
    }
}
void SymbolTable::visit(ArrayAccessNode& node) {
    if (node.array_expr) node.array_expr->accept(*this);
    if (node.index_expr) node.index_expr->accept(*this);
}
void SymbolTable::visit(MemberAccessNode& node) {
    if (node.object_expr) node.object_expr->accept(*this);
}
void SymbolTable::visit(AddressOfNode& node) {
    if (node.operand) node.operand->accept(*this);
}
void SymbolTable::visit(DereferenceNode& node) {
    if (node.operand) node.operand->accept(*this);
}

// Statement visitors
void SymbolTable::visit(ExpressionStatementNode& node) {
    if (node.expression) node.expression->accept(*this);
}

void SymbolTable::visit(CompoundStatementNode& node) {
    enter_scope(ScopeKind::BLOCK, "block");
    node.scope = current_scope_;
    for (auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
    exit_scope();
}

void SymbolTable::visit(IfStatementNode& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.then_branch) node.then_branch->accept(*this);
    if (node.else_branch) node.else_branch->accept(*this);
}

void SymbolTable::visit(WhileStatementNode& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
}

void SymbolTable::visit(ForStatementNode& node) {
    enter_scope(ScopeKind::BLOCK, "for_loop");
    node.scope = current_scope_;
    if (node.init) node.init->accept(*this);
    if (node.condition) node.condition->accept(*this);
    if (node.update) node.update->accept(*this);
    if (node.body) node.body->accept(*this);
    exit_scope();
}

void SymbolTable::visit(ReturnStatementNode& node) {
    if (node.expression) node.expression->accept(*this);
}

// Declaration visitors
void SymbolTable::visit(VariableDeclarationNode& node) {
    TypePtr var_type = node.type;
    if (node.is_array) {
        var_type = Type::make_array(node.type, node.array_size);
    }
    auto sym = Symbol::make_variable(node.name, var_type, node.location);
    insert(sym);

    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

void SymbolTable::visit(ParameterNode& node) {
    auto sym = Symbol::make_parameter(node.name, node.type, node.location);
    insert(sym);
}

void SymbolTable::visit(FunctionDeclarationNode& node) {
    std::vector<TypePtr> param_types;
    for (const auto& p : node.params) {
        param_types.push_back(p ? p->type : Type::make_void());
    }
    TypePtr func_type = Type::make_function(node.return_type, param_types);

    auto sym = Symbol::make_function(node.name, func_type, node.location);
    for (const auto& p : node.params) {
        if (p) {
            sym->parameters.push_back(Symbol::make_parameter(p->name, p->type, p->location));
        }
    }
    insert(sym);
}

void SymbolTable::visit(FunctionDefinitionNode& node) {
    std::vector<TypePtr> param_types;
    for (const auto& p : node.params) {
        param_types.push_back(p ? p->type : Type::make_void());
    }
    TypePtr func_type = Type::make_function(node.return_type, param_types);

    auto existing = lookup_current(node.name);
    if (existing) {
        if (existing->is_function() && existing->type && existing->type->equals(*func_type)) {
            // Forward declaration match, allow definition
        } else {
            record_error("redefinition of function '" + node.name + "'", node.location);
        }
    } else {
        auto sym = Symbol::make_function(node.name, func_type, node.location);
        for (const auto& p : node.params) {
            if (p) {
                sym->parameters.push_back(Symbol::make_parameter(p->name, p->type, p->location));
            }
        }
        insert(sym);
    }

    // Function body scope
    enter_scope(ScopeKind::FUNCTION, node.name);
    node.scope = current_scope_;
    for (auto& p : node.params) {
        if (p) p->accept(*this);
    }
    if (node.body) {
        for (auto& stmt : node.body->statements) {
            if (stmt) stmt->accept(*this);
        }
    }
    exit_scope();
}

void SymbolTable::visit(FieldDeclarationNode&) {
    // Handled inside ClassDeclarationNode
}

void SymbolTable::visit(ClassDeclarationNode& node) {
    auto class_sym = Symbol::make_class(node.name, node.location);
    if (!insert(class_sym)) {
        return;
    }

    enter_scope(ScopeKind::CLASS, node.name);
    node.scope = current_scope_;
    for (auto& field : node.fields) {
        if (field) {
            TypePtr f_type = field->type;
            if (field->is_array) {
                f_type = Type::make_array(field->type, field->array_size);
            }
            auto f_sym = Symbol::make_field(field->name, f_type, field->location);
            if (!class_sym->add_field(f_sym)) {
                record_error("duplicate field '" + field->name + "' in class '" + node.name + "'", field->location);
            }
            insert(f_sym);
        }
    }
    exit_scope();
}

void SymbolTable::visit(ProgramNode& node) {
    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }
}

} // namespace tacco
