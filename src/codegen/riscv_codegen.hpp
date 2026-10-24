#ifndef TACCO_RISCV_CODEGEN_HPP
#define TACCO_RISCV_CODEGEN_HPP

#include "../ir/ir.hpp"
#include "regalloc.hpp"
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

namespace tacco {

class RISCVCodeGenerator {
public:
    RISCVCodeGenerator();

    std::string generate(const IRProgram& program);
    void generate_to_stream(const IRProgram& program, std::ostream& os);

private:
    void emit_program_header(const IRProgram& program, std::ostream& os);
    void emit_rodata_section(const IRProgram& program, std::ostream& os);
    void emit_function(const IRFunction& fn, const IRProgram& program, std::ostream& os);

    // Helpers
    std::string load_operand_to_reg(const Operand& op, const std::string& reg,
                                    const std::map<std::string, int>& var_offsets,
                                    const LinearScanAllocator& regalloc,
                                    std::ostream& os);

    void store_reg_to_dest(const Operand& dest, const std::string& reg,
                           const std::map<std::string, int>& var_offsets,
                           const LinearScanAllocator& regalloc,
                           std::ostream& os);
};

} // namespace tacco

#endif // TACCO_RISCV_CODEGEN_HPP
