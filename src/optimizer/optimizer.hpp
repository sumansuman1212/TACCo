#ifndef TACCO_OPTIMIZER_HPP
#define TACCO_OPTIMIZER_HPP

#include "../ir/ir.hpp"
#include "../cfg/cfg.hpp"
#include <memory>
#include <vector>
#include <map>
#include <set>

namespace tacco {

class Optimizer {
public:
    Optimizer() = default;

    // Optimize full program
    void optimize(IRProgram& program);

    // Optimize single function
    bool optimize_function(IRFunction& fn);

    // Individual optimization passes
    static bool run_constant_folding(IRFunction& fn);
    static bool run_algebraic_simplification(IRFunction& fn);
    static bool run_constant_propagation(IRFunction& fn);
    static bool run_common_subexpression_elimination(IRFunction& fn);
    static bool run_dead_code_elimination(IRFunction& fn);
    static bool run_peephole_optimization(IRFunction& fn);

private:
    int max_iterations_ = 10;
};

} // namespace tacco

#endif // TACCO_OPTIMIZER_HPP
