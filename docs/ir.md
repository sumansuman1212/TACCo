# TACCo Three Address Code (3AC) Specification

## Instruction Set
TACCo lowers the typed AST into explicit 3AC quadruples/triples:

- `t = a + b` (Binary arithmetic: `ADD`, `SUB`, `MUL`, `DIV`, `MOD`)
- `t = -a`, `t = !a` (Unary operations)
- `t = a == b` (Comparisons: `EQ`, `NE`, `LT`, `LE`, `GT`, `GE`)
- `t = c` (Constant assignment)
- `dest = src` (Copy assignment)
- `goto L1` (Unconditional jump)
- `if_true t goto L1` / `if_false t goto L1` (Conditional branches)
- `label L1` (Label target)
- `param x`, `call func, n`, `return x` (Procedure calls)
- `t = &x` (Address of)
- `t = *p` (Load through pointer)
- `*p = t` (Store through pointer)
- `t = arr[i]` (Array index load)
- `arr[i] = t` (Array index store)
