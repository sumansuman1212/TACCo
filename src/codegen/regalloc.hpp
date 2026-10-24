#ifndef TACCO_REGALLOC_HPP
#define TACCO_REGALLOC_HPP

#include "../ir/ir.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>

namespace tacco {

struct LiveInterval {
    std::string name;
    int start_point = -1;
    int end_point = -1;
    std::string reg;
    int spill_offset = 0;
    bool is_spilled = false;

    LiveInterval() = default;
    explicit LiveInterval(std::string n) : name(std::move(n)) {}
};

class LinearScanAllocator {
public:
    LinearScanAllocator();

    void allocate(const IRFunction& fn, int initial_stack_offset = 0);

    const std::map<std::string, std::string>& get_allocation() const { return var_to_reg_; }
    const std::map<std::string, int>& get_spill_offsets() const { return var_to_spill_; }
    int get_spill_size() const { return total_spill_size_; }

    std::string get_register_for(const std::string& name) const;
    int get_spill_offset_for(const std::string& name) const;
    bool is_spilled(const std::string& name) const;

    void dump(std::ostream& os) const;

private:
    std::vector<std::string> available_registers_;
    std::map<std::string, std::string> var_to_reg_;
    std::map<std::string, int> var_to_spill_;
    std::vector<LiveInterval> intervals_;
    int total_spill_size_ = 0;
};

} // namespace tacco

#endif // TACCO_REGALLOC_HPP
