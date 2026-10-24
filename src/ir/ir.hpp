#ifndef TACCO_IR_HPP
#define TACCO_IR_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <map>
#include "../ast/types.hpp"
#include "../common/location.hpp"

namespace tacco {

enum class OperandKind {
    NONE,
    CONSTANT,
    TEMPORARY,
    VARIABLE,
    LABEL
};

class Operand {
public:
    OperandKind kind = OperandKind::NONE;
    int64_t val = 0;             // For CONSTANT
    std::string name;            // For TEMPORARY, VARIABLE, LABEL, or string literal name
    TypePtr type = nullptr;      // Inferred or associated type

    Operand() = default;

    static Operand make_none() {
        return Operand();
    }

    static Operand make_const(int64_t v, TypePtr t = nullptr) {
        Operand op;
        op.kind = OperandKind::CONSTANT;
        op.val = v;
        op.type = t ? std::move(t) : Type::make_int();
        return op;
    }

    static Operand make_temp(const std::string& temp_name, TypePtr t = nullptr) {
        Operand op;
        op.kind = OperandKind::TEMPORARY;
        op.name = temp_name;
        op.type = std::move(t);
        return op;
    }

    static Operand make_var(const std::string& var_name, TypePtr t = nullptr) {
        Operand op;
        op.kind = OperandKind::VARIABLE;
        op.name = var_name;
        op.type = std::move(t);
        return op;
    }

    static Operand make_label(const std::string& lbl_name) {
        Operand op;
        op.kind = OperandKind::LABEL;
        op.name = lbl_name;
        return op;
    }

    bool is_none() const { return kind == OperandKind::NONE; }
    bool is_const() const { return kind == OperandKind::CONSTANT; }
    bool is_temp() const { return kind == OperandKind::TEMPORARY; }
    bool is_var() const { return kind == OperandKind::VARIABLE; }
    bool is_label() const { return kind == OperandKind::LABEL; }

    bool equals(const Operand& other) const {
        if (kind != other.kind) return false;
        if (kind == OperandKind::CONSTANT) return val == other.val;
        if (kind == OperandKind::NONE) return true;
        return name == other.name;
    }

    std::string to_string() const {
        switch (kind) {
            case OperandKind::NONE: return "";
            case OperandKind::CONSTANT: return std::to_string(val);
            case OperandKind::TEMPORARY: return name;
            case OperandKind::VARIABLE: return name;
            case OperandKind::LABEL: return name;
        }
        return "";
    }
};

enum class Opcode {
    // Arithmetic
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,

    // Relational / Comparison
    EQ,
    NE,
    LT,
    LE,
    GT,
    GE,

    // Unary & Assignment
    ASSIGN,
    NEG,
    NOT,

    // Control Flow
    LABEL,
    GOTO,
    IF_TRUE,
    IF_FALSE,

    // Functions
    PARAM,
    CALL,
    RETURN,

    // Memory / Addressing
    ADDR_OF,     // dest = &src1
    LOAD_PTR,    // dest = *src1
    STORE_PTR,   // *dest = src1
    LOAD_ARR,    // dest = src1[src2]
    STORE_ARR,   // dest[src1] = src2
    LOAD_FIELD,  // dest = src1.offset (offset in src2 const)
    STORE_FIELD  // dest.offset = src1 (offset in src2 const)
};

const char* opcode_to_string(Opcode op);

class Instruction {
public:
    Opcode op;
    Operand dest;
    Operand src1;
    Operand src2;
    SourceLocation location;

    Instruction(Opcode o, Operand d, Operand s1 = Operand::make_none(), Operand s2 = Operand::make_none(), SourceLocation loc = SourceLocation())
        : op(o), dest(std::move(d)), src1(std::move(s1)), src2(std::move(s2)), location(std::move(loc)) {}

    std::string to_string() const;
};

struct LocalVarInfo {
    std::string name;
    TypePtr type;
    int size = 8;
    int offset = 0; // Stack offset from frame pointer
};

struct ClassInfo {
    std::string name;
    int total_size = 0;
    std::map<std::string, int> field_offsets;
    std::map<std::string, TypePtr> field_types;
};

class IRFunction {
public:
    std::string name;
    TypePtr return_type;
    std::vector<LocalVarInfo> params;
    std::vector<LocalVarInfo> locals;
    std::vector<Instruction> instructions;
    int stack_frame_size = 0;

    IRFunction(std::string n, TypePtr ret)
        : name(std::move(n)), return_type(std::move(ret)) {}

    void add_instruction(Instruction inst) {
        instructions.push_back(std::move(inst));
    }

    void dump(std::ostream& os) const;
};

class IRProgram {
public:
    std::vector<IRFunction> functions;
    std::map<std::string, ClassInfo> classes;
    std::vector<std::string> string_literals;

    void add_function(IRFunction fn) {
        functions.push_back(std::move(fn));
    }

    void dump(std::ostream& os) const;
};

} // namespace tacco

#endif // TACCO_IR_HPP
