#ifndef TACCO_CFG_HPP
#define TACCO_CFG_HPP

#include "../ir/ir.hpp"
#include <vector>
#include <memory>
#include <string>
#include <set>
#include <map>
#include <iostream>

namespace tacco {

class BasicBlock {
public:
    int id = 0;
    std::string label;
    std::vector<Instruction> instructions;
    std::vector<BasicBlock*> predecessors;
    std::vector<BasicBlock*> successors;

    BasicBlock(int bid, std::string lbl) : id(bid), label(std::move(lbl)) {}

    void add_instruction(Instruction inst) {
        instructions.push_back(std::move(inst));
    }

    void add_successor(BasicBlock* succ) {
        for (auto* s : successors) {
            if (s == succ) return;
        }
        successors.push_back(succ);
        succ->predecessors.push_back(this);
    }

    std::string to_string() const;
    void dump(std::ostream& os) const;
};

class ControlFlowGraph {
public:
    std::string function_name;
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    BasicBlock* entry_block = nullptr;

    explicit ControlFlowGraph(const std::string& fn_name) : function_name(fn_name) {}

    static std::unique_ptr<ControlFlowGraph> build_from_function(const IRFunction& fn);

    std::string to_dot() const;
    void dump(std::ostream& os) const;
};

class ProgramCFG {
public:
    std::map<std::string, std::unique_ptr<ControlFlowGraph>> function_cfgs;

    static std::unique_ptr<ProgramCFG> build_from_program(const IRProgram& program);

    std::string to_dot() const;
    void dump(std::ostream& os) const;
};

} // namespace tacco

#endif // TACCO_CFG_HPP
