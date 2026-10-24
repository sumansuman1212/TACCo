#include "optimizer.hpp"
#include <algorithm>
#include <tuple>

namespace tacco {

void Optimizer::optimize(IRProgram& program) {
    for (auto& fn : program.functions) {
        for (int iter = 0; iter < max_iterations_; ++iter) {
            bool changed = optimize_function(fn);
            if (!changed) break;
        }
    }
}

bool Optimizer::optimize_function(IRFunction& fn) {
    bool changed = false;
    changed |= run_constant_folding(fn);
    changed |= run_algebraic_simplification(fn);
    changed |= run_constant_propagation(fn);
    changed |= run_common_subexpression_elimination(fn);
    changed |= run_dead_code_elimination(fn);
    changed |= run_peephole_optimization(fn);
    return changed;
}

bool Optimizer::run_constant_folding(IRFunction& fn) {
    bool changed = false;

    for (auto& inst : fn.instructions) {
        // Binary operations with both operands constant
        if (inst.src1.is_const() && inst.src2.is_const()) {
            int64_t v1 = inst.src1.val;
            int64_t v2 = inst.src2.val;
            int64_t res = 0;
            bool folded = true;

            switch (inst.op) {
                case Opcode::ADD: res = v1 + v2; break;
                case Opcode::SUB: res = v1 - v2; break;
                case Opcode::MUL: res = v1 * v2; break;
                case Opcode::DIV:
                    if (v2 != 0) res = v1 / v2;
                    else folded = false;
                    break;
                case Opcode::MOD:
                    if (v2 != 0) res = v1 % v2;
                    else folded = false;
                    break;
                case Opcode::EQ: res = (v1 == v2) ? 1 : 0; break;
                case Opcode::NE: res = (v1 != v2) ? 1 : 0; break;
                case Opcode::LT: res = (v1 < v2) ? 1 : 0; break;
                case Opcode::LE: res = (v1 <= v2) ? 1 : 0; break;
                case Opcode::GT: res = (v1 > v2) ? 1 : 0; break;
                case Opcode::GE: res = (v1 >= v2) ? 1 : 0; break;
                default: folded = false; break;
            }

            if (folded) {
                inst.op = Opcode::ASSIGN;
                inst.src1 = Operand::make_const(res, inst.dest.type);
                inst.src2 = Operand::make_none();
                changed = true;
            }
        }
        // Unary operations on constant
        else if (inst.src1.is_const() && inst.src2.is_none()) {
            if (inst.op == Opcode::NEG) {
                inst.op = Opcode::ASSIGN;
                inst.src1 = Operand::make_const(-inst.src1.val, inst.dest.type);
                changed = true;
            } else if (inst.op == Opcode::NOT) {
                inst.op = Opcode::ASSIGN;
                inst.src1 = Operand::make_const(inst.src1.val == 0 ? 1 : 0, inst.dest.type);
                changed = true;
            }
        }
    }

    return changed;
}

bool Optimizer::run_algebraic_simplification(IRFunction& fn) {
    bool changed = false;

    for (auto& inst : fn.instructions) {
        // x + 0 -> x
        if (inst.op == Opcode::ADD) {
            if (inst.src2.is_const() && inst.src2.val == 0) {
                inst.op = Opcode::ASSIGN;
                inst.src2 = Operand::make_none();
                changed = true;
            } else if (inst.src1.is_const() && inst.src1.val == 0) {
                inst.op = Opcode::ASSIGN;
                inst.src1 = inst.src2;
                inst.src2 = Operand::make_none();
                changed = true;
            }
        }
        // x - 0 -> x, x - x -> 0
        else if (inst.op == Opcode::SUB) {
            if (inst.src2.is_const() && inst.src2.val == 0) {
                inst.op = Opcode::ASSIGN;
                inst.src2 = Operand::make_none();
                changed = true;
            } else if (inst.src1.equals(inst.src2) && !inst.src1.is_none()) {
                inst.op = Opcode::ASSIGN;
                inst.src1 = Operand::make_const(0, inst.dest.type);
                inst.src2 = Operand::make_none();
                changed = true;
            }
        }
        // x * 1 -> x, x * 0 -> 0
        else if (inst.op == Opcode::MUL) {
            if (inst.src2.is_const()) {
                if (inst.src2.val == 1) {
                    inst.op = Opcode::ASSIGN;
                    inst.src2 = Operand::make_none();
                    changed = true;
                } else if (inst.src2.val == 0) {
                    inst.op = Opcode::ASSIGN;
                    inst.src1 = Operand::make_const(0, inst.dest.type);
                    inst.src2 = Operand::make_none();
                    changed = true;
                }
            } else if (inst.src1.is_const()) {
                if (inst.src1.val == 1) {
                    inst.op = Opcode::ASSIGN;
                    inst.src1 = inst.src2;
                    inst.src2 = Operand::make_none();
                    changed = true;
                } else if (inst.src1.val == 0) {
                    inst.op = Opcode::ASSIGN;
                    inst.src1 = Operand::make_const(0, inst.dest.type);
                    inst.src2 = Operand::make_none();
                    changed = true;
                }
            }
        }
        // x / 1 -> x
        else if (inst.op == Opcode::DIV) {
            if (inst.src2.is_const() && inst.src2.val == 1) {
                inst.op = Opcode::ASSIGN;
                inst.src2 = Operand::make_none();
                changed = true;
            }
        }
    }

    return changed;
}

bool Optimizer::run_constant_propagation(IRFunction& fn) {
    bool changed = false;
    std::map<std::string, Operand> const_map;

    for (auto& inst : fn.instructions) {
        // Invalidate on control-flow targets or function calls
        if (inst.op == Opcode::LABEL || inst.op == Opcode::CALL) {
            const_map.clear();
            continue;
        }

        // Replace src1 if constant
        if (inst.src1.is_temp() || inst.src1.is_var()) {
            auto it = const_map.find(inst.src1.name);
            if (it != const_map.end() && it->second.is_const()) {
                inst.src1 = it->second;
                changed = true;
            }
        }

        // Replace src2 if constant
        if (inst.src2.is_temp() || inst.src2.is_var()) {
            auto it = const_map.find(inst.src2.name);
            if (it != const_map.end() && it->second.is_const()) {
                inst.src2 = it->second;
                changed = true;
            }
        }

        // Record or invalidate dest
        if (!inst.dest.is_none()) {
            if (inst.op == Opcode::ASSIGN && inst.src1.is_const()) {
                const_map[inst.dest.name] = inst.src1;
            } else {
                const_map.erase(inst.dest.name);
            }
        }

        // If storing through pointer or array, clear variable mappings
        if (inst.op == Opcode::STORE_PTR || inst.op == Opcode::STORE_ARR || inst.op == Opcode::STORE_FIELD) {
            // Keep only temporaries
            for (auto it = const_map.begin(); it != const_map.end();) {
                if (it->second.is_var()) {
                    it = const_map.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    return changed;
}

bool Optimizer::run_common_subexpression_elimination(IRFunction& fn) {
    bool changed = false;
    using ExprKey = std::tuple<int, std::string, std::string>;
    std::map<ExprKey, Operand> available_exprs;

    for (auto& inst : fn.instructions) {
        // Invalidate at labels or calls
        if (inst.op == Opcode::LABEL || inst.op == Opcode::CALL ||
            inst.op == Opcode::STORE_PTR || inst.op == Opcode::STORE_ARR || inst.op == Opcode::STORE_FIELD) {
            available_exprs.clear();
            continue;
        }

        // Check if instruction is a binary expression
        bool is_binop = (inst.op == Opcode::ADD || inst.op == Opcode::SUB ||
                         inst.op == Opcode::MUL || inst.op == Opcode::DIV ||
                         inst.op == Opcode::MOD || inst.op == Opcode::EQ ||
                         inst.op == Opcode::NE || inst.op == Opcode::LT ||
                         inst.op == Opcode::LE || inst.op == Opcode::GT ||
                         inst.op == Opcode::GE);

        if (is_binop && !inst.dest.is_none()) {
            ExprKey key{static_cast<int>(inst.op), inst.src1.to_string(), inst.src2.to_string()};
            auto it = available_exprs.find(key);
            if (it != available_exprs.end()) {
                // Reuse previous definition
                inst.op = Opcode::ASSIGN;
                inst.src1 = it->second;
                inst.src2 = Operand::make_none();
                changed = true;
            } else {
                available_exprs[key] = inst.dest;
            }
        }

        // If an operand is modified, remove expressions using it
        if (!inst.dest.is_none()) {
            std::string dest_str = inst.dest.to_string();
            for (auto it = available_exprs.begin(); it != available_exprs.end();) {
                if (std::get<1>(it->first) == dest_str || std::get<2>(it->first) == dest_str ||
                    it->second.to_string() == dest_str) {
                    it = available_exprs.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    return changed;
}

bool Optimizer::run_dead_code_elimination(IRFunction& fn) {
    bool changed = false;

    // Count uses of temporaries and variables
    std::map<std::string, int> use_counts;
    for (const auto& inst : fn.instructions) {
        if (!inst.src1.is_none() && (inst.src1.is_temp() || inst.src1.is_var())) {
            use_counts[inst.src1.name]++;
        }
        if (!inst.src2.is_none() && (inst.src2.is_temp() || inst.src2.is_var())) {
            use_counts[inst.src2.name]++;
        }
    }

    std::vector<Instruction> new_insts;
    bool unreachable = false;

    for (size_t i = 0; i < fn.instructions.size(); ++i) {
        auto& inst = fn.instructions[i];

        // Labels reset unreachable state
        if (inst.op == Opcode::LABEL) {
            unreachable = false;
        }

        if (unreachable) {
            changed = true;
            continue;
        }

        // Simplify constant branches: if_true 1 goto L -> goto L; if_false 0 goto L -> goto L
        if (inst.op == Opcode::IF_TRUE && inst.src1.is_const()) {
            if (inst.src1.val != 0) {
                inst.op = Opcode::GOTO;
                inst.src1 = Operand::make_none();
                changed = true;
            } else {
                // Branch never taken
                changed = true;
                continue;
            }
        } else if (inst.op == Opcode::IF_FALSE && inst.src1.is_const()) {
            if (inst.src1.val == 0) {
                inst.op = Opcode::GOTO;
                inst.src1 = Operand::make_none();
                changed = true;
            } else {
                // Branch never taken
                changed = true;
                continue;
            }
        }

        // Dead temporary definition elimination (only for side-effect-free instructions)
        bool has_side_effects = (inst.op == Opcode::CALL || inst.op == Opcode::STORE_PTR ||
                                 inst.op == Opcode::STORE_ARR || inst.op == Opcode::STORE_FIELD ||
                                 inst.op == Opcode::RETURN || inst.op == Opcode::GOTO ||
                                 inst.op == Opcode::IF_TRUE || inst.op == Opcode::IF_FALSE ||
                                 inst.op == Opcode::LABEL || inst.op == Opcode::PARAM);

        if (!has_side_effects && (inst.dest.is_temp() || inst.dest.is_var())) {
            if (use_counts[inst.dest.name] == 0) {
                changed = true;
                continue; // Drop dead instruction
            }
        }

        new_insts.push_back(inst);

        // Mark subsequent instructions as unreachable after unconditional GOTO or RETURN
        if (inst.op == Opcode::GOTO || inst.op == Opcode::RETURN) {
            unreachable = true;
        }
    }

    fn.instructions = std::move(new_insts);
    return changed;
}

bool Optimizer::run_peephole_optimization(IRFunction& fn) {
    bool changed = false;
    std::vector<Instruction> new_insts;

    for (size_t i = 0; i < fn.instructions.size(); ++i) {
        const auto& inst = fn.instructions[i];

        // Peephole 1: Eliminate self assignment x = x
        if (inst.op == Opcode::ASSIGN && inst.dest.equals(inst.src1)) {
            changed = true;
            continue;
        }

        // Peephole 2: Eliminate goto L immediately followed by label L
        if (inst.op == Opcode::GOTO && i + 1 < fn.instructions.size()) {
            const auto& next = fn.instructions[i + 1];
            if (next.op == Opcode::LABEL && next.dest.equals(inst.dest)) {
                changed = true;
                continue; // Skip the redundant goto
            }
        }

        new_insts.push_back(inst);
    }

    fn.instructions = std::move(new_insts);
    return changed;
}

} // namespace tacco
