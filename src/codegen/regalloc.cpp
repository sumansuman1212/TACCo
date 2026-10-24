#include "regalloc.hpp"
#include <algorithm>

namespace tacco {

LinearScanAllocator::LinearScanAllocator() {
    // RV64 temporary registers
    available_registers_ = {
        "t2", "t3", "t4", "t5", "t6"
    };
}

void LinearScanAllocator::allocate(const IRFunction& fn, int initial_stack_offset) {
    var_to_reg_.clear();
    var_to_spill_.clear();
    intervals_.clear();
    total_spill_size_ = 0;

    std::map<std::string, LiveInterval> interval_map;

    // Step 1: Compute live intervals from instructions
    for (int idx = 0; idx < static_cast<int>(fn.instructions.size()); ++idx) {
        const auto& inst = fn.instructions[idx];

        auto record_use = [&](const Operand& op) {
            if (op.is_temp() || op.is_var()) {
                if (interval_map.find(op.name) == interval_map.end()) {
                    interval_map[op.name] = LiveInterval(op.name);
                    interval_map[op.name].start_point = idx;
                }
                interval_map[op.name].end_point = idx;
            }
        };

        record_use(inst.src1);
        record_use(inst.src2);
        record_use(inst.dest);
    }

    for (auto& kv : interval_map) {
        intervals_.push_back(kv.second);
    }

    // Step 2: Sort intervals by start_point
    std::sort(intervals_.begin(), intervals_.end(), [](const LiveInterval& a, const LiveInterval& b) {
        return a.start_point < b.start_point;
    });

    // Step 3: Linear scan allocation
    std::set<std::string> free_pool(available_registers_.begin(), available_registers_.end());
    std::vector<LiveInterval*> active;
    int current_spill_offset = initial_stack_offset;

    for (auto& current : intervals_) {
        // Expire old intervals
        active.erase(std::remove_if(active.begin(), active.end(), [&](LiveInterval* act) {
            if (act->end_point < current.start_point) {
                if (!act->is_spilled && !act->reg.empty()) {
                    free_pool.insert(act->reg);
                }
                return true;
            }
            return false;
        }), active.end());

        // Allocate register or spill
        if (!free_pool.empty()) {
            std::string assigned_reg = *free_pool.begin();
            free_pool.erase(free_pool.begin());
            current.reg = assigned_reg;
            current.is_spilled = false;
            var_to_reg_[current.name] = assigned_reg;
            active.push_back(&current);
        } else {
            // Spill to stack
            current_spill_offset += 8;
            current.is_spilled = true;
            current.spill_offset = current_spill_offset;
            var_to_spill_[current.name] = current_spill_offset;
            total_spill_size_ += 8;
        }
    }
}

std::string LinearScanAllocator::get_register_for(const std::string& name) const {
    auto it = var_to_reg_.find(name);
    if (it != var_to_reg_.end()) {
        return it->second;
    }
    return "";
}

int LinearScanAllocator::get_spill_offset_for(const std::string& name) const {
    auto it = var_to_spill_.find(name);
    if (it != var_to_spill_.end()) {
        return it->second;
    }
    return 0;
}

bool LinearScanAllocator::is_spilled(const std::string& name) const {
    return var_to_spill_.find(name) != var_to_spill_.end();
}

void LinearScanAllocator::dump(std::ostream& os) const {
    os << "=== Register Allocation Map ===\n";
    for (const auto& kv : var_to_reg_) {
        os << "  " << kv.first << " -> " << kv.second << "\n";
    }
    if (!var_to_spill_.empty()) {
        os << "=== Spill Offsets ===\n";
        for (const auto& kv : var_to_spill_) {
            os << "  " << kv.first << " -> -" << kv.second << "(s0)\n";
        }
    }
}

} // namespace tacco
