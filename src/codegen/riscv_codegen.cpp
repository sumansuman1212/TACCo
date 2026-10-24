#include "riscv_codegen.hpp"
#include <iomanip>

namespace tacco {

RISCVCodeGenerator::RISCVCodeGenerator() = default;

std::string RISCVCodeGenerator::generate(const IRProgram& program) {
    std::ostringstream ss;
    generate_to_stream(program, ss);
    return ss.str();
}

void RISCVCodeGenerator::generate_to_stream(const IRProgram& program, std::ostream& os) {
    emit_program_header(program, os);
    emit_rodata_section(program, os);

    os << "    .text\n";
    for (const auto& fn : program.functions) {
        emit_function(fn, program, os);
    }
}

void RISCVCodeGenerator::emit_program_header(const IRProgram&, std::ostream& os) {
    os << "    .file   \"tacco_output.tc\"\n";
    os << "    .option nopic\n";
    os << "    .attribute arch, \"rv64gc\"\n";
    os << "    .attribute unaligned_access, 0\n";
    os << "    .attribute stack_align, 16\n\n";
}

void RISCVCodeGenerator::emit_rodata_section(const IRProgram& program, std::ostream& os) {
    if (program.string_literals.empty()) return;

    os << "    .section .rodata\n";
    for (size_t i = 0; i < program.string_literals.size(); ++i) {
        os << ".LC_str_" << i << ":\n";
        os << "    .string \"" << program.string_literals[i] << "\"\n";
    }
    os << "\n";
}

std::string RISCVCodeGenerator::load_operand_to_reg(const Operand& op, const std::string& target_reg,
                                                    const std::map<std::string, int>& var_offsets,
                                                    const LinearScanAllocator& regalloc,
                                                    std::ostream& os) {
    if (op.is_const()) {
        os << "    li " << target_reg << ", " << op.val << "\n";
        return target_reg;
    }

    if (op.is_label()) {
        os << "    lla " << target_reg << ", " << op.name << "\n";
        return target_reg;
    }

    // Named variables: always stored in/loaded from stack frame
    if (op.is_var()) {
        auto it = var_offsets.find(op.name);
        if (it != var_offsets.end()) {
            os << "    ld " << target_reg << ", -" << it->second << "(s0)\n";
            return target_reg;
        }
    }

    // Virtual temporaries: check register allocator or spill slot
    if (op.is_temp()) {
        std::string phys_reg = regalloc.get_register_for(op.name);
        if (!phys_reg.empty()) {
            if (phys_reg != target_reg) {
                os << "    mv " << target_reg << ", " << phys_reg << "\n";
            }
            return target_reg;
        }

        int spill = regalloc.get_spill_offset_for(op.name);
        if (spill > 0) {
            os << "    ld " << target_reg << ", -" << spill << "(s0)\n";
            return target_reg;
        }
    }

    // Fallback: 0
    os << "    li " << target_reg << ", 0\n";
    return target_reg;
}

void RISCVCodeGenerator::store_reg_to_dest(const Operand& dest, const std::string& src_reg,
                                           const std::map<std::string, int>& var_offsets,
                                           const LinearScanAllocator& regalloc,
                                           std::ostream& os) {
    if (dest.is_none()) return;

    if (dest.is_var()) {
        auto it = var_offsets.find(dest.name);
        if (it != var_offsets.end()) {
            os << "    sd " << src_reg << ", -" << it->second << "(s0)\n";
            return;
        }
    }

    if (dest.is_temp()) {
        std::string phys_reg = regalloc.get_register_for(dest.name);
        if (!phys_reg.empty()) {
            if (phys_reg != src_reg) {
                os << "    mv " << phys_reg << ", " << src_reg << "\n";
            }
            return;
        }

        int spill = regalloc.get_spill_offset_for(dest.name);
        if (spill > 0) {
            os << "    sd " << src_reg << ", -" << spill << "(s0)\n";
            return;
        }
    }
}

void RISCVCodeGenerator::emit_function(const IRFunction& fn, const IRProgram&, std::ostream& os) {
    os << "    .globl  " << fn.name << "\n";
    os << "    .type   " << fn.name << ", @function\n";
    os << fn.name << ":\n";

    // Compute stack layout
    std::map<std::string, int> var_offsets;
    int current_offset = 16; // ra at 8, s0 at 16

    // Parameter offsets
    for (const auto& p : fn.params) {
        current_offset += 8;
        var_offsets[p.name] = current_offset;
    }

    // Local variable offsets (scalars, arrays, structs)
    for (const auto& l : fn.locals) {
        int sz = (l.size + 7) & ~7;
        current_offset += sz;
        var_offsets[l.name] = current_offset;
    }

    // Run register allocation
    LinearScanAllocator regalloc;
    regalloc.allocate(fn, current_offset);
    current_offset += regalloc.get_spill_size();

    // Align frame size to 16 bytes
    int frame_size = (current_offset + 15) & ~15;
    if (frame_size < 32) frame_size = 32;

    std::string ret_label = ".L_ret_" + fn.name;

    // Function Prologue
    os << "    addi sp, sp, -" << frame_size << "\n";
    os << "    sd ra, " << (frame_size - 8) << "(sp)\n";
    os << "    sd s0, " << (frame_size - 16) << "(sp)\n";
    os << "    addi s0, sp, " << frame_size << "\n";

    // Store incoming parameters (a0 - a7) to their stack offsets
    const char* arg_regs[] = {"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
    for (size_t i = 0; i < fn.params.size() && i < 8; ++i) {
        int off = var_offsets[fn.params[i].name];
        os << "    sd " << arg_regs[i] << ", -" << off << "(s0)\n";
    }

    // Parameter collection buffer for function calls
    std::vector<Operand> call_params;

    // Lower instructions
    for (const auto& inst : fn.instructions) {
        switch (inst.op) {
            case Opcode::LABEL:
                os << inst.dest.name << ":\n";
                break;

            case Opcode::GOTO:
                os << "    j " << inst.dest.name << "\n";
                break;

            case Opcode::IF_TRUE:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                os << "    bnez t0, " << inst.dest.name << "\n";
                break;

            case Opcode::IF_FALSE:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                os << "    beqz t0, " << inst.dest.name << "\n";
                break;

            case Opcode::PARAM:
                call_params.push_back(inst.src1);
                break;

            case Opcode::CALL: {
                for (size_t i = 0; i < call_params.size() && i < 8; ++i) {
                    load_operand_to_reg(call_params[i], arg_regs[i], var_offsets, regalloc, os);
                }
                call_params.clear();
                os << "    call " << inst.src1.name << "\n";
                if (!inst.dest.is_none()) {
                    store_reg_to_dest(inst.dest, "a0", var_offsets, regalloc, os);
                }
                break;
            }

            case Opcode::RETURN:
                if (!inst.src1.is_none()) {
                    load_operand_to_reg(inst.src1, "a0", var_offsets, regalloc, os);
                }
                os << "    j " << ret_label << "\n";
                break;

            case Opcode::ASSIGN:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::NEG:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                os << "    neg t0, t0\n";
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::NOT:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                os << "    seqz t0, t0\n";
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::ADD:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    add t0, t0, t1\n";
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::SUB:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    sub t0, t0, t1\n";
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::MUL:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    mul t0, t0, t1\n";
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::DIV:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    div t0, t0, t1\n";
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::MOD:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    rem t0, t0, t1\n";
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::EQ:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    sub t0, t0, t1\n";
                os << "    seqz t0, t0\n";
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::NE:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    sub t0, t0, t1\n";
                os << "    snez t0, t0\n";
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::LT:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    slt t0, t0, t1\n";
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::LE:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    slt t0, t1, t0\n"; // t1 < t0
                os << "    xori t0, t0, 1\n"; // !(t1 < t0) <=> t0 <= t1
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::GT:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    slt t0, t1, t0\n"; // t0 > t1 <=> t1 < t0
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::GE:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    slt t0, t0, t1\n"; // t0 < t1
                os << "    xori t0, t0, 1\n"; // !(t0 < t1) <=> t0 >= t1
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;

            case Opcode::ADDR_OF: {
                auto it = var_offsets.find(inst.src1.name);
                if (it != var_offsets.end()) {
                    os << "    addi t0, s0, -" << it->second << "\n";
                } else {
                    os << "    mv t0, s0\n";
                }
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;
            }

            case Opcode::LOAD_PTR:
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                os << "    ld t1, 0(t0)\n";
                store_reg_to_dest(inst.dest, "t1", var_offsets, regalloc, os);
                break;

            case Opcode::STORE_PTR:
                load_operand_to_reg(inst.dest, "t0", var_offsets, regalloc, os);
                load_operand_to_reg(inst.src1, "t1", var_offsets, regalloc, os);
                os << "    sd t1, 0(t0)\n";
                break;

            case Opcode::LOAD_ARR: {
                auto it = var_offsets.find(inst.src1.name);
                if (it != var_offsets.end()) {
                    os << "    addi t0, s0, -" << it->second << "\n";
                } else {
                    load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                }
                load_operand_to_reg(inst.src2, "t1", var_offsets, regalloc, os);
                os << "    slli t1, t1, 3\n"; // Multiply index by 8
                os << "    add t0, t0, t1\n";
                os << "    ld t2, 0(t0)\n";
                store_reg_to_dest(inst.dest, "t2", var_offsets, regalloc, os);
                break;
            }

            case Opcode::STORE_ARR: {
                auto it = var_offsets.find(inst.dest.name);
                if (it != var_offsets.end()) {
                    os << "    addi t0, s0, -" << it->second << "\n";
                } else {
                    load_operand_to_reg(inst.dest, "t0", var_offsets, regalloc, os);
                }
                load_operand_to_reg(inst.src1, "t1", var_offsets, regalloc, os); // index
                load_operand_to_reg(inst.src2, "t2", var_offsets, regalloc, os); // value
                os << "    slli t1, t1, 3\n"; // Multiply index by 8
                os << "    add t0, t0, t1\n";
                os << "    sd t2, 0(t0)\n";
                break;
            }

            case Opcode::LOAD_FIELD: {
                int field_offset = static_cast<int>(inst.src2.val);
                auto it = var_offsets.find(inst.src1.name);
                if (it != var_offsets.end()) {
                    int offset = it->second - field_offset;
                    os << "    ld t0, -" << offset << "(s0)\n";
                } else {
                    load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os);
                    os << "    ld t0, " << field_offset << "(t0)\n";
                }
                store_reg_to_dest(inst.dest, "t0", var_offsets, regalloc, os);
                break;
            }

            case Opcode::STORE_FIELD: {
                int field_offset = static_cast<int>(inst.src2.val);
                load_operand_to_reg(inst.src1, "t0", var_offsets, regalloc, os); // value
                auto it = var_offsets.find(inst.dest.name);
                if (it != var_offsets.end()) {
                    int offset = it->second - field_offset;
                    os << "    sd t0, -" << offset << "(s0)\n";
                } else {
                    load_operand_to_reg(inst.dest, "t1", var_offsets, regalloc, os);
                    os << "    sd t0, " << field_offset << "(t1)\n";
                }
                break;
            }
        }
    }

    // Function Epilogue
    os << ret_label << ":\n";
    os << "    ld ra, " << (frame_size - 8) << "(sp)\n";
    os << "    ld s0, " << (frame_size - 16) << "(sp)\n";
    os << "    addi sp, sp, " << frame_size << "\n";
    os << "    ret\n";
    os << "    .size   " << fn.name << ", .-" << fn.name << "\n\n";
}

} // namespace tacco
