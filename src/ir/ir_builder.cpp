#include "ir_builder.hpp"

namespace tacco {

IRBuilder::IRBuilder() = default;

std::string IRBuilder::new_temp() {
    return "t" + std::to_string(temp_counter_++);
}

std::string IRBuilder::new_label() {
    return "L" + std::to_string(label_counter_++);
}

int IRBuilder::calculate_type_size(const TypePtr& type) {
    if (!type) return 8;
    if (type->is_char()) return 1;
    if (type->is_int() || type->is_pointer()) return 8;
    if (type->is_array()) {
        int elem_sz = calculate_type_size(type->base_type);
        return elem_sz * type->array_size;
    }
    if (type->is_class()) {
        auto it = program_->classes.find(type->name);
        if (it != program_->classes.end()) {
            return it->second.total_size;
        }
        return 8;
    }
    return 8;
}

Operand IRBuilder::evaluate_expr(ExpressionNode& expr) {
    expr.accept(*this);
    return last_operand_;
}

std::unique_ptr<IRProgram> IRBuilder::build(ProgramNode& program) {
    program_ = std::make_unique<IRProgram>();
    temp_counter_ = 0;
    label_counter_ = 0;
    str_counter_ = 0;

    program.accept(*this);
    return std::move(program_);
}

void IRBuilder::visit(ProgramNode& node) {
    // First collect class declarations
    for (const auto& decl : node.declarations) {
        if (auto class_decl = dynamic_cast<ClassDeclarationNode*>(decl.get())) {
            class_decl->accept(*this);
        }
    }

    // Then process functions and global variables
    for (const auto& decl : node.declarations) {
        if (!dynamic_cast<ClassDeclarationNode*>(decl.get())) {
            decl->accept(*this);
        }
    }
}

void IRBuilder::visit(ClassDeclarationNode& node) {
    ClassInfo info;
    info.name = node.name;
    int current_offset = 0;

    for (const auto& field : node.fields) {
        int fsize = 8;
        if (field->is_array) {
            int elem_sz = (field->type && field->type->is_char()) ? 1 : 8;
            fsize = elem_sz * field->array_size;
        } else if (field->type) {
            fsize = calculate_type_size(field->type);
        }
        // Align to 8 bytes if needed
        if (current_offset % 8 != 0 && fsize >= 8) {
            current_offset = (current_offset + 7) & ~7;
        }
        info.field_offsets[field->name] = current_offset;
        info.field_types[field->name] = field->type;
        current_offset += fsize;
    }

    // Round total size up to multiple of 8
    if (current_offset % 8 != 0) {
        current_offset = (current_offset + 7) & ~7;
    }
    info.total_size = current_offset;
    program_->classes[node.name] = std::move(info);
}

void IRBuilder::visit(FieldDeclarationNode&) {}

void IRBuilder::visit(FunctionDeclarationNode&) {}

void IRBuilder::visit(FunctionDefinitionNode& node) {
    IRFunction fn(node.name, node.return_type);
    current_fn_ = &fn;

    for (const auto& param : node.params) {
        LocalVarInfo pinfo;
        pinfo.name = param->name;
        pinfo.type = param->type;
        pinfo.size = calculate_type_size(param->type);
        current_fn_->params.push_back(pinfo);
    }

    if (node.body) {
        node.body->accept(*this);
    }

    // Ensure function has a return statement at the end
    if (current_fn_->instructions.empty() ||
        current_fn_->instructions.back().op != Opcode::RETURN) {
        current_fn_->add_instruction(Instruction(Opcode::RETURN, Operand::make_none(), Operand::make_none(), Operand::make_none(), node.location));
    }

    program_->add_function(std::move(fn));
    current_fn_ = nullptr;
}

void IRBuilder::visit(VariableDeclarationNode& node) {
    if (!current_fn_) return;

    LocalVarInfo linfo;
    linfo.name = node.name;
    linfo.type = node.type;

    if (node.is_array) {
        int elem_sz = (node.type && node.type->is_char()) ? 1 : 8;
        linfo.size = elem_sz * node.array_size;
    } else {
        linfo.size = calculate_type_size(node.type);
    }

    current_fn_->locals.push_back(linfo);

    if (node.initializer) {
        Operand val_op = evaluate_expr(*node.initializer);
        Operand var_op = Operand::make_var(node.name, node.type);
        current_fn_->add_instruction(Instruction(Opcode::ASSIGN, var_op, val_op, Operand::make_none(), node.location));
    }
}

void IRBuilder::visit(ParameterNode&) {}

void IRBuilder::visit(CompoundStatementNode& node) {
    for (const auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void IRBuilder::visit(ExpressionStatementNode& node) {
    if (node.expression) {
        evaluate_expr(*node.expression);
    }
}

void IRBuilder::visit(IfStatementNode& node) {
    Operand cond = evaluate_expr(*node.condition);
    std::string lbl_else = new_label();
    std::string lbl_end = new_label();

    if (node.else_branch) {
        current_fn_->add_instruction(Instruction(Opcode::IF_FALSE, Operand::make_label(lbl_else), cond, Operand::make_none(), node.location));
        if (node.then_branch) {
            node.then_branch->accept(*this);
        }
        current_fn_->add_instruction(Instruction(Opcode::GOTO, Operand::make_label(lbl_end), Operand::make_none(), Operand::make_none(), node.location));
        current_fn_->add_instruction(Instruction(Opcode::LABEL, Operand::make_label(lbl_else), Operand::make_none(), Operand::make_none(), node.location));
        node.else_branch->accept(*this);
        current_fn_->add_instruction(Instruction(Opcode::LABEL, Operand::make_label(lbl_end), Operand::make_none(), Operand::make_none(), node.location));
    } else {
        current_fn_->add_instruction(Instruction(Opcode::IF_FALSE, Operand::make_label(lbl_end), cond, Operand::make_none(), node.location));
        if (node.then_branch) {
            node.then_branch->accept(*this);
        }
        current_fn_->add_instruction(Instruction(Opcode::LABEL, Operand::make_label(lbl_end), Operand::make_none(), Operand::make_none(), node.location));
    }
}

void IRBuilder::visit(WhileStatementNode& node) {
    std::string lbl_cond = new_label();
    std::string lbl_end = new_label();

    current_fn_->add_instruction(Instruction(Opcode::LABEL, Operand::make_label(lbl_cond), Operand::make_none(), Operand::make_none(), node.location));
    Operand cond = evaluate_expr(*node.condition);
    current_fn_->add_instruction(Instruction(Opcode::IF_FALSE, Operand::make_label(lbl_end), cond, Operand::make_none(), node.location));

    if (node.body) {
        node.body->accept(*this);
    }

    current_fn_->add_instruction(Instruction(Opcode::GOTO, Operand::make_label(lbl_cond), Operand::make_none(), Operand::make_none(), node.location));
    current_fn_->add_instruction(Instruction(Opcode::LABEL, Operand::make_label(lbl_end), Operand::make_none(), Operand::make_none(), node.location));
}

void IRBuilder::visit(ForStatementNode& node) {
    if (node.init) {
        node.init->accept(*this);
    }

    std::string lbl_cond = new_label();
    std::string lbl_end = new_label();

    current_fn_->add_instruction(Instruction(Opcode::LABEL, Operand::make_label(lbl_cond), Operand::make_none(), Operand::make_none(), node.location));

    if (node.condition) {
        Operand cond = evaluate_expr(*node.condition);
        current_fn_->add_instruction(Instruction(Opcode::IF_FALSE, Operand::make_label(lbl_end), cond, Operand::make_none(), node.location));
    }

    if (node.body) {
        node.body->accept(*this);
    }

    if (node.update) {
        evaluate_expr(*node.update);
    }

    current_fn_->add_instruction(Instruction(Opcode::GOTO, Operand::make_label(lbl_cond), Operand::make_none(), Operand::make_none(), node.location));
    current_fn_->add_instruction(Instruction(Opcode::LABEL, Operand::make_label(lbl_end), Operand::make_none(), Operand::make_none(), node.location));
}

void IRBuilder::visit(ReturnStatementNode& node) {
    if (node.expression) {
        Operand ret_val = evaluate_expr(*node.expression);
        current_fn_->add_instruction(Instruction(Opcode::RETURN, Operand::make_none(), ret_val, Operand::make_none(), node.location));
    } else {
        current_fn_->add_instruction(Instruction(Opcode::RETURN, Operand::make_none(), Operand::make_none(), Operand::make_none(), node.location));
    }
}

void IRBuilder::visit(IntegerLiteralNode& node) {
    last_operand_ = Operand::make_const(node.value, node.evaluated_type);
}

void IRBuilder::visit(CharLiteralNode& node) {
    last_operand_ = Operand::make_const(static_cast<int64_t>(node.value), node.evaluated_type);
}

void IRBuilder::visit(StringLiteralNode& node) {
    std::string str_lbl = ".LC_str_" + std::to_string(str_counter_++);
    program_->string_literals.push_back(node.value);
    last_operand_ = Operand::make_label(str_lbl);
}

void IRBuilder::visit(VariableAccessNode& node) {
    last_operand_ = Operand::make_var(node.name, node.evaluated_type);
}

void IRBuilder::visit(BinaryExpressionNode& node) {
    if (node.op == BinaryOp::LOGICAL_AND) {
        std::string lbl_false = new_label();
        std::string lbl_end = new_label();
        std::string res_temp = new_temp();
        Operand res_op = Operand::make_temp(res_temp, Type::make_int());

        Operand left = evaluate_expr(*node.left);
        current_fn_->add_instruction(Instruction(Opcode::IF_FALSE, Operand::make_label(lbl_false), left, Operand::make_none(), node.location));
        
        Operand right = evaluate_expr(*node.right);
        current_fn_->add_instruction(Instruction(Opcode::IF_FALSE, Operand::make_label(lbl_false), right, Operand::make_none(), node.location));

        current_fn_->add_instruction(Instruction(Opcode::ASSIGN, res_op, Operand::make_const(1), Operand::make_none(), node.location));
        current_fn_->add_instruction(Instruction(Opcode::GOTO, Operand::make_label(lbl_end), Operand::make_none(), Operand::make_none(), node.location));
        current_fn_->add_instruction(Instruction(Opcode::LABEL, Operand::make_label(lbl_false), Operand::make_none(), Operand::make_none(), node.location));
        current_fn_->add_instruction(Instruction(Opcode::ASSIGN, res_op, Operand::make_const(0), Operand::make_none(), node.location));
        current_fn_->add_instruction(Instruction(Opcode::LABEL, Operand::make_label(lbl_end), Operand::make_none(), Operand::make_none(), node.location));

        last_operand_ = res_op;
        return;
    }

    if (node.op == BinaryOp::LOGICAL_OR) {
        std::string lbl_true = new_label();
        std::string lbl_end = new_label();
        std::string res_temp = new_temp();
        Operand res_op = Operand::make_temp(res_temp, Type::make_int());

        Operand left = evaluate_expr(*node.left);
        current_fn_->add_instruction(Instruction(Opcode::IF_TRUE, Operand::make_label(lbl_true), left, Operand::make_none(), node.location));

        Operand right = evaluate_expr(*node.right);
        current_fn_->add_instruction(Instruction(Opcode::IF_TRUE, Operand::make_label(lbl_true), right, Operand::make_none(), node.location));

        current_fn_->add_instruction(Instruction(Opcode::ASSIGN, res_op, Operand::make_const(0), Operand::make_none(), node.location));
        current_fn_->add_instruction(Instruction(Opcode::GOTO, Operand::make_label(lbl_end), Operand::make_none(), Operand::make_none(), node.location));
        current_fn_->add_instruction(Instruction(Opcode::LABEL, Operand::make_label(lbl_true), Operand::make_none(), Operand::make_none(), node.location));
        current_fn_->add_instruction(Instruction(Opcode::ASSIGN, res_op, Operand::make_const(1), Operand::make_none(), node.location));
        current_fn_->add_instruction(Instruction(Opcode::LABEL, Operand::make_label(lbl_end), Operand::make_none(), Operand::make_none(), node.location));

        last_operand_ = res_op;
        return;
    }

    Operand left_op = evaluate_expr(*node.left);
    Operand right_op = evaluate_expr(*node.right);

    Opcode opc;
    switch (node.op) {
        case BinaryOp::ADD: opc = Opcode::ADD; break;
        case BinaryOp::SUB: opc = Opcode::SUB; break;
        case BinaryOp::MUL: opc = Opcode::MUL; break;
        case BinaryOp::DIV: opc = Opcode::DIV; break;
        case BinaryOp::MOD: opc = Opcode::MOD; break;
        case BinaryOp::EQ: opc = Opcode::EQ; break;
        case BinaryOp::NE: opc = Opcode::NE; break;
        case BinaryOp::LT: opc = Opcode::LT; break;
        case BinaryOp::LE: opc = Opcode::LE; break;
        case BinaryOp::GT: opc = Opcode::GT; break;
        case BinaryOp::GE: opc = Opcode::GE; break;
        default: opc = Opcode::ADD; break;
    }

    std::string t = new_temp();
    Operand dest = Operand::make_temp(t, node.evaluated_type);
    current_fn_->add_instruction(Instruction(opc, dest, left_op, right_op, node.location));
    last_operand_ = dest;
}

void IRBuilder::visit(UnaryExpressionNode& node) {
    if (node.op == UnaryOp::PLUS) {
        last_operand_ = evaluate_expr(*node.operand);
        return;
    }

    Operand src = evaluate_expr(*node.operand);
    std::string t = new_temp();
    Operand dest = Operand::make_temp(t, node.evaluated_type);

    if (node.op == UnaryOp::MINUS) {
        current_fn_->add_instruction(Instruction(Opcode::NEG, dest, src, Operand::make_none(), node.location));
    } else if (node.op == UnaryOp::LOGICAL_NOT) {
        current_fn_->add_instruction(Instruction(Opcode::NOT, dest, src, Operand::make_none(), node.location));
    }

    last_operand_ = dest;
}

void IRBuilder::visit(AssignmentNode& node) {
    Operand val_op = evaluate_expr(*node.value);

    // Case 1: Variable assignment
    if (auto var_node = dynamic_cast<VariableAccessNode*>(node.target.get())) {
        Operand var_op = Operand::make_var(var_node->name, var_node->evaluated_type);
        current_fn_->add_instruction(Instruction(Opcode::ASSIGN, var_op, val_op, Operand::make_none(), node.location));
        last_operand_ = var_op;
        return;
    }

    // Case 2: Dereference assignment *p = val
    if (auto deref_node = dynamic_cast<DereferenceNode*>(node.target.get())) {
        Operand ptr_op = evaluate_expr(*deref_node->operand);
        current_fn_->add_instruction(Instruction(Opcode::STORE_PTR, ptr_op, val_op, Operand::make_none(), node.location));
        last_operand_ = val_op;
        return;
    }

    // Case 3: Array assignment arr[i] = val
    if (auto arr_node = dynamic_cast<ArrayAccessNode*>(node.target.get())) {
        Operand arr_op = evaluate_expr(*arr_node->array_expr);
        Operand idx_op = evaluate_expr(*arr_node->index_expr);
        current_fn_->add_instruction(Instruction(Opcode::STORE_ARR, arr_op, idx_op, val_op, node.location));
        last_operand_ = val_op;
        return;
    }

    // Case 4: Member assignment obj.field = val
    if (auto mem_node = dynamic_cast<MemberAccessNode*>(node.target.get())) {
        Operand obj_op = evaluate_expr(*mem_node->object_expr);
        std::string class_name = mem_node->object_expr->evaluated_type ? mem_node->object_expr->evaluated_type->name : "";
        int offset = 0;
        auto it = program_->classes.find(class_name);
        if (it != program_->classes.end()) {
            auto fit = it->second.field_offsets.find(mem_node->member_name);
            if (fit != it->second.field_offsets.end()) {
                offset = fit->second;
            }
        }
        current_fn_->add_instruction(Instruction(Opcode::STORE_FIELD, obj_op, val_op, Operand::make_const(offset), node.location));
        last_operand_ = val_op;
        return;
    }

    last_operand_ = val_op;
}

void IRBuilder::visit(FunctionCallNode& node) {
    std::vector<Operand> arg_ops;
    for (const auto& arg : node.args) {
        arg_ops.push_back(evaluate_expr(*arg));
    }

    for (const auto& op : arg_ops) {
        current_fn_->add_instruction(Instruction(Opcode::PARAM, Operand::make_none(), op, Operand::make_none(), node.location));
    }

    Operand callee_op = Operand::make_var(node.callee_name);
    Operand count_op = Operand::make_const(static_cast<int64_t>(node.args.size()));

    if (node.evaluated_type && !node.evaluated_type->is_void()) {
        std::string t = new_temp();
        Operand dest = Operand::make_temp(t, node.evaluated_type);
        current_fn_->add_instruction(Instruction(Opcode::CALL, dest, callee_op, count_op, node.location));
        last_operand_ = dest;
    } else {
        current_fn_->add_instruction(Instruction(Opcode::CALL, Operand::make_none(), callee_op, count_op, node.location));
        last_operand_ = Operand::make_none();
    }
}

void IRBuilder::visit(ArrayAccessNode& node) {
    Operand arr_op = evaluate_expr(*node.array_expr);
    Operand idx_op = evaluate_expr(*node.index_expr);

    std::string t = new_temp();
    Operand dest = Operand::make_temp(t, node.evaluated_type);
    current_fn_->add_instruction(Instruction(Opcode::LOAD_ARR, dest, arr_op, idx_op, node.location));
    last_operand_ = dest;
}

void IRBuilder::visit(MemberAccessNode& node) {
    Operand obj_op = evaluate_expr(*node.object_expr);
    std::string class_name = node.object_expr->evaluated_type ? node.object_expr->evaluated_type->name : "";
    int offset = 0;
    auto it = program_->classes.find(class_name);
    if (it != program_->classes.end()) {
        auto fit = it->second.field_offsets.find(node.member_name);
        if (fit != it->second.field_offsets.end()) {
            offset = fit->second;
        }
    }

    std::string t = new_temp();
    Operand dest = Operand::make_temp(t, node.evaluated_type);
    current_fn_->add_instruction(Instruction(Opcode::LOAD_FIELD, dest, obj_op, Operand::make_const(offset), node.location));
    last_operand_ = dest;
}

void IRBuilder::visit(AddressOfNode& node) {
    if (auto var_node = dynamic_cast<VariableAccessNode*>(node.operand.get())) {
        std::string t = new_temp();
        Operand dest = Operand::make_temp(t, node.evaluated_type);
        current_fn_->add_instruction(Instruction(Opcode::ADDR_OF, dest, Operand::make_var(var_node->name), Operand::make_none(), node.location));
        last_operand_ = dest;
    } else {
        Operand src = evaluate_expr(*node.operand);
        std::string t = new_temp();
        Operand dest = Operand::make_temp(t, node.evaluated_type);
        current_fn_->add_instruction(Instruction(Opcode::ADDR_OF, dest, src, Operand::make_none(), node.location));
        last_operand_ = dest;
    }
}

void IRBuilder::visit(DereferenceNode& node) {
    Operand ptr_op = evaluate_expr(*node.operand);
    std::string t = new_temp();
    Operand dest = Operand::make_temp(t, node.evaluated_type);
    current_fn_->add_instruction(Instruction(Opcode::LOAD_PTR, dest, ptr_op, Operand::make_none(), node.location));
    last_operand_ = dest;
}

} // namespace tacco
