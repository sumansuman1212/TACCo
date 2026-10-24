#include "cfg.hpp"
#include <sstream>

namespace tacco {

std::string BasicBlock::to_string() const {
    std::ostringstream ss;
    ss << "Block " << id << " (" << label << "):\n";
    for (const auto& inst : instructions) {
        ss << "  " << inst.to_string() << "\n";
    }
    return ss.str();
}

void BasicBlock::dump(std::ostream& os) const {
    os << to_string();
    os << "  Predecessors: ";
    for (size_t i = 0; i < predecessors.size(); ++i) {
        if (i > 0) os << ", ";
        os << "B" << predecessors[i]->id;
    }
    os << "\n  Successors: ";
    for (size_t i = 0; i < successors.size(); ++i) {
        if (i > 0) os << ", ";
        os << "B" << successors[i]->id;
    }
    os << "\n";
}

std::unique_ptr<ControlFlowGraph> ControlFlowGraph::build_from_function(const IRFunction& fn) {
    auto cfg = std::make_unique<ControlFlowGraph>(fn.name);
    if (fn.instructions.empty()) return cfg;

    const auto& insts = fn.instructions;
    size_t n = insts.size();

    // Step 1: Leader identification
    std::vector<bool> is_leader(n, false);
    is_leader[0] = true;

    for (size_t i = 0; i < n; ++i) {
        const auto& inst = insts[i];
        if (inst.op == Opcode::LABEL) {
            is_leader[i] = true;
        } else if (inst.op == Opcode::GOTO || inst.op == Opcode::IF_TRUE ||
                   inst.op == Opcode::IF_FALSE || inst.op == Opcode::RETURN) {
            if (i + 1 < n) {
                is_leader[i + 1] = true;
            }
        }
    }

    // Step 2: Form basic blocks
    std::map<std::string, BasicBlock*> label_to_block;
    std::vector<BasicBlock*> block_ptrs;

    int block_id = 0;
    std::unique_ptr<BasicBlock> current_bb = nullptr;

    for (size_t i = 0; i < n; ++i) {
        if (is_leader[i]) {
            if (current_bb) {
                block_ptrs.push_back(current_bb.get());
                cfg->blocks.push_back(std::move(current_bb));
            }
            std::string blk_label = "B" + std::to_string(block_id);
            if (insts[i].op == Opcode::LABEL) {
                blk_label = insts[i].dest.to_string();
            }
            current_bb = std::make_unique<BasicBlock>(block_id++, blk_label);
        }

        if (insts[i].op == Opcode::LABEL) {
            label_to_block[insts[i].dest.to_string()] = current_bb.get();
        }
        current_bb->add_instruction(insts[i]);
    }

    if (current_bb) {
        block_ptrs.push_back(current_bb.get());
        cfg->blocks.push_back(std::move(current_bb));
    }

    if (!cfg->blocks.empty()) {
        cfg->entry_block = cfg->blocks[0].get();
    }

    // Step 3: Connect edges between basic blocks
    for (size_t i = 0; i < cfg->blocks.size(); ++i) {
        BasicBlock* bb = cfg->blocks[i].get();
        if (bb->instructions.empty()) continue;

        const auto& last_inst = bb->instructions.back();

        if (last_inst.op == Opcode::GOTO) {
            std::string target_label = last_inst.dest.to_string();
            auto it = label_to_block.find(target_label);
            if (it != label_to_block.end()) {
                bb->add_successor(it->second);
            }
        } else if (last_inst.op == Opcode::IF_TRUE || last_inst.op == Opcode::IF_FALSE) {
            std::string target_label = last_inst.dest.to_string();
            auto it = label_to_block.find(target_label);
            if (it != label_to_block.end()) {
                bb->add_successor(it->second);
            }
            // Fallthrough edge
            if (i + 1 < cfg->blocks.size()) {
                bb->add_successor(cfg->blocks[i + 1].get());
            }
        } else if (last_inst.op == Opcode::RETURN) {
            // Function return: no fallthrough
        } else {
            // Normal sequential fallthrough
            if (i + 1 < cfg->blocks.size()) {
                bb->add_successor(cfg->blocks[i + 1].get());
            }
        }
    }

    return cfg;
}

std::string ControlFlowGraph::to_dot() const {
    std::ostringstream ss;
    ss << "digraph CFG_" << function_name << " {\n";
    ss << "  node [shape=box, fontname=\"Courier\", style=\"filled\", fillcolor=\"#F0F4F8\"];\n";
    ss << "  edge [fontname=\"Helvetica\"];\n\n";

    for (const auto& bb : blocks) {
        ss << "  node_" << bb->id << " [label=\"" << bb->label << "\\n";
        for (const auto& inst : bb->instructions) {
            std::string line = inst.to_string();
            // Escape quotes and newlines for DOT
            std::string escaped;
            for (char c : line) {
                if (c == '"') escaped += "\\\"";
                else if (c == '\n') escaped += "\\n";
                else escaped += c;
            }
            ss << escaped << "\\l";
        }
        ss << "\"];\n";
    }

    ss << "\n";
    for (const auto& bb : blocks) {
        for (const auto* succ : bb->successors) {
            ss << "  node_" << bb->id << " -> node_" << succ->id << ";\n";
        }
    }

    ss << "}\n";
    return ss.str();
}

void ControlFlowGraph::dump(std::ostream& os) const {
    os << "=== Control Flow Graph for " << function_name << " ===\n";
    for (const auto& bb : blocks) {
        bb->dump(os);
        os << "\n";
    }
}

std::unique_ptr<ProgramCFG> ProgramCFG::build_from_program(const IRProgram& program) {
    auto pcfg = std::make_unique<ProgramCFG>();
    for (const auto& fn : program.functions) {
        pcfg->function_cfgs[fn.name] = ControlFlowGraph::build_from_function(fn);
    }
    return pcfg;
}

std::string ProgramCFG::to_dot() const {
    std::ostringstream ss;
    for (const auto& kv : function_cfgs) {
        ss << kv.second->to_dot() << "\n";
    }
    return ss.str();
}

void ProgramCFG::dump(std::ostream& os) const {
    for (const auto& kv : function_cfgs) {
        kv.second->dump(os);
    }
}

} // namespace tacco
