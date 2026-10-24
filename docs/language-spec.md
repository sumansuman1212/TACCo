# TACCo Language Specification (v0.1)

## 1. Lexical Structure

### 1.1 Keywords
The following identifiers are reserved as keywords:
`int`, `char`, `void`, `if`, `else`, `while`, `for`, `return`, `class`

### 1.2 Identifiers
Identifiers start with a letter or underscore, followed by zero or more letters, digits, or underscores:
`[a-zA-Z_][a-zA-Z0-9_]*`

### 1.3 Literals
- **Integer Literals**: Decimal (`[0-9]+`) and Hexadecimal (`0[xX][0-9a-fA-F]+`).
- **Character Literals**: Enclosed in single quotes with escape support (`'a'`, `'\n'`, `'\t'`, `'\0'`, `'\\'`, `'\''`).
- **String Literals**: Enclosed in double quotes (`"string"`).

### 1.4 Operators & Punctuation
- **Arithmetic**: `+`, `-`, `*`, `/`, `%`
- **Relational**: `==`, `!=`, `<`, `>`, `<=`, `>=`
- **Logical**: `&&`, `||`, `!`
- **Assignment**: `=`
- **Memory & Access**: `&` (address-of), `*` (dereference / multiplication), `.` (field access)
- **Delimiters**: `(`, `)`, `{`, `}`, `[`, `]`, `;`, `,`

### 1.5 Comments & Whitespace
- Line comments begin with `//` and extend to the end of the line.
- Block comments are enclosed in `/* ... */`.
- Whitespace (spaces, tabs, newlines) acts as token delimiters.

---

## 2. Type System
- Primitive Types: `int` (4/8 bytes in RV64), `char` (1 byte), `void`
- Pointer Types: `T*`
- Array Types: `T[N]`
- Class Types: User-defined class types with named fields.
