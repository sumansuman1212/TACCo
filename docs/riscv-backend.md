# TACCo RISC-V Backend Specification

## Target Architecture: RV64GC / RV64I

### Register Set & ABI Conventions
- `zero` (`x0`): Hardwired zero
- `ra` (`x1`): Return address
- `sp` (`x2`): Stack pointer (16-byte aligned)
- `gp` (`x3`), `tp` (`x4`): Global and thread pointers
- `t0`-`t2` (`x5`-`x7`), `t3`-`t6` (`x28`-`x31`): Caller-saved temporaries
- `s0`/`fp` (`x8`): Frame pointer / saved register
- `s1` (`x9`), `s2`-`s11` (`x18`-`x27`): Callee-saved registers
- `a0`-`a1` (`x10`-`x11`): Function arguments & return values
- `a2`-`a7` (`x12`-`x17`): Function arguments

### Stack Frame Layout
```
+--------------------------+  Higher addresses
| Previous Frame / Args    |
+--------------------------+
| Saved RA                 |
+--------------------------+
| Saved FP                 |
+--------------------------+
| Callee-saved Registers   |
+--------------------------+
| Local Variables & Arrays |
+--------------------------+
| Spilled Temporaries      |
+--------------------------+  <- SP (16-byte aligned)
```
