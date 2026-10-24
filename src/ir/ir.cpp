#include "ir.hpp"
#include <iomanip>

namespace tacco {

const char* opcode_to_string(Opcode op) {
    switch (op) {
        case Opcode::ADD: return "ADD";
        case Opcode::SUB: return "SUB";
        case Opcode::MUL: return "MUL";
        case Opcode::DIV: return "DIV";
        case Opcode::MOD: return "MOD";
        case Opcode::EQ: return "EQ";
        case Opcode::NE: return "NE";
        case Opcode::LT: return "LT";
        case Opcode::LE: return "LE";
        case Opcode::GT: return "GT";
        case Opcode::GE: return "GE";
        case Opcode::ASSIGN: return "ASSIGN";
        case Opcode::NEG: return "NEG";
        case Opcode::NOT: return "NOT";
        case Opcode::LABEL: return "LABEL";
        case Opcode::GOTO: return "GOTO";
        case Opcode::IF_TRUE: return "IF_TRUE";
        case Opcode::IF_FALSE: return "IF_FALSE";
        case Opcode::PARAM: return "PARAM";
        case Opcode::CALL: return "CALL";
        case Opcode::RETURN: return "RETURN";
        case Opcode::ADDR_OF: return "ADDR_OF";
        case Opcode::LOAD_PTR: return "LOAD_PTR";
        case Opcode::STORE_PTR: return "STORE_PTR";
        case Opcode::LOAD_ARR: return "LOAD_ARR";
        case Opcode::STORE_ARR: return "STORE_ARR";
        case Opcode::LOAD_FIELD: return "LOAD_FIELD";
        case Opcode::STORE_FIELD: return "STORE_FIELD";
    }
    return "UNKNOWN";
}

std::string Instruction::to_string() const {
    std::ostringstream ss;
    switch (op) {
        case Opcode::LABEL:
            ss << dest.to_string() << ":";
            break;
        case Opcode::GOTO:
            ss << "    goto " << dest.to_string();
            break;
        case Opcode::IF_TRUE:
            ss << "    if_true " << src1.to_string() << " goto " << dest.to_string();
            break;
        case Opcode::IF_FALSE:
            ss << "    if_false " << src1.to_string() << " goto " << dest.to_string();
            break;
        case Opcode::PARAM:
            ss << "    param " << src1.to_string();
            break;
        case Opcode::CALL:
            if (!dest.is_none()) {
                ss << "    " << dest.to_string() << " = call " << src1.to_string() << ", " << src2.to_string();
            } else {
                ss << "    call " << src1.to_string() << ", " << src2.to_string();
            }
            break;
        case Opcode::RETURN:
            if (!src1.is_none()) {
                ss << "    return " << src1.to_string();
            } else {
                ss << "    return";
            }
            break;
        case Opcode::ASSIGN:
            ss << "    " << dest.to_string() << " = " << src1.to_string();
            break;
        case Opcode::NEG:
            ss << "    " << dest.to_string() << " = -" << src1.to_string();
            break;
        case Opcode::NOT:
            ss << "    " << dest.to_string() << " = !" << src1.to_string();
            break;
        case Opcode::ADDR_OF:
            ss << "    " << dest.to_string() << " = &" << src1.to_string();
            break;
        case Opcode::LOAD_PTR:
            ss << "    " << dest.to_string() << " = *" << src1.to_string();
            break;
        case Opcode::STORE_PTR:
            ss << "    *" << dest.to_string() << " = " << src1.to_string();
            break;
        case Opcode::LOAD_ARR:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << "[" << src2.to_string() << "]";
            break;
        case Opcode::STORE_ARR:
            ss << "    " << dest.to_string() << "[" << src1.to_string() << "] = " << src2.to_string();
            break;
        case Opcode::LOAD_FIELD:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " + " << src2.to_string() << " (field)";
            break;
        case Opcode::STORE_FIELD:
            ss << "    " << dest.to_string() << " + " << src2.to_string() << " = " << src1.to_string() << " (field)";
            break;
        case Opcode::ADD:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " + " << src2.to_string();
            break;
        case Opcode::SUB:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " - " << src2.to_string();
            break;
        case Opcode::MUL:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " * " << src2.to_string();
            break;
        case Opcode::DIV:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " / " << src2.to_string();
            break;
        case Opcode::MOD:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " % " << src2.to_string();
            break;
        case Opcode::EQ:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " == " << src2.to_string();
            break;
        case Opcode::NE:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " != " << src2.to_string();
            break;
        case Opcode::LT:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " < " << src2.to_string();
            break;
        case Opcode::LE:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " <= " << src2.to_string();
            break;
        case Opcode::GT:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " > " << src2.to_string();
            break;
        case Opcode::GE:
            ss << "    " << dest.to_string() << " = " << src1.to_string() << " >= " << src2.to_string();
            break;
    }
    return ss.str();
}

void IRFunction::dump(std::ostream& os) const {
    os << "function " << name << "(";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) os << ", ";
        os << params[i].name;
    }
    os << ") -> " << (return_type ? return_type->to_string() : "void") << " {\n";
    
    if (!locals.empty()) {
        os << "  // Locals:\n";
        for (const auto& l : locals) {
            os << "  //   " << l.name << " (" << (l.type ? l.type->to_string() : "unknown") << ", " << l.size << " bytes)\n";
        }
    }

    for (const auto& inst : instructions) {
        os << inst.to_string() << "\n";
    }
    os << "}\n";
}

void IRProgram::dump(std::ostream& os) const {
    if (!classes.empty()) {
        os << "=== Classes & Memory Layouts ===\n";
        for (const auto& kv : classes) {
            const auto& ci = kv.second;
            os << "class " << ci.name << " (" << ci.total_size << " bytes):\n";
            for (const auto& f : ci.field_offsets) {
                auto it_type = ci.field_types.find(f.first);
                std::string tstr = (it_type != ci.field_types.end() && it_type->second) ? it_type->second->to_string() : "unknown";
                os << "  +" << f.second << " : " << f.first << " (" << tstr << ")\n";
            }
        }
        os << "\n";
    }

    os << "=== 3AC IR Functions ===\n";
    for (const auto& fn : functions) {
        fn.dump(os);
        os << "\n";
    }
}

} // namespace tacco
