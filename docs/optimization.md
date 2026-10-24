# TACCo Optimization Passes

## Optimization Framework

TACCo applies control-flow graph (CFG) and basic-block level optimization passes on Three Address Code (3AC):

1. **Constant Folding**: Evaluates compile-time constant arithmetic and comparison expressions.
2. **Constant Propagation**: Propagates known constant values into downstream uses within and across basic blocks.
3. **Algebraic Simplification**: Simplifies algebraic identities (`x + 0 -> x`, `x * 1 -> x`, `x * 0 -> 0`).
4. **Dead Code Elimination (DCE)**: Removes unused temporary definitions and unreachable basic blocks.
5. **Common Subexpression Elimination (CSE)**: Identifies redundant computations and reuses previously computed values.
6. **Peephole Optimization**: Local branch simplification and redundant instruction elimination.
