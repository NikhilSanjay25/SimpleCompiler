# SimpleCompiler

A simple compiler built using **Flex**, **Bison**, and **LLVM** that supports arithmetic expressions, variable assignment, and a `print` statement.

## What it supports

- Integer variables
- Assignment: `x = 5;`
- Arithmetic: `+`, `-`, `*`, `/` with correct operator precedence
- Parentheses: `(1 + 2) * 3`
- Print statement: `print x;`

---

## Project structure

```
SimpleCompiler/
├── ast.c        # AST node constructors
├── ast.h        # AST node type definitions
├── codegen.c    # LLVM IR code generation
├── lexer.l      # Flex lexer (tokeniser)
├── parser.y     # Bison parser (grammar + AST builder)
├── Makefile     # Build system
└── run.sh       # One-shot compile and run script
```

---

## Setup

### 1. Install dependencies

```bash
sudo apt update
sudo apt install flex bison clang llvm-dev -y
```

Verify LLVM is installed:

```bash
llvm-config --version
```

### 2. Clone the repo

```bash
git clone https://github.com/NikhilSanjay25/SimpleCompiler.git
cd SimpleCompiler
```

### 3. Build the compiler

```bash
make
```

This runs flex and bison to generate the lexer and parser, then compiles everything into a `compiler` binary.

---

## Usage

### Option A — Pipe a program directly

```bash
echo "x = 42; print x;" | ./compiler
llc out.ll -filetype=obj -relocation-model=pic -o out.o
clang -fPIE out.o -o myprog
./myprog
```

### Option B — Write a `.txt` file and run it

Create a file `myprogram.txt`:

```
x = 10;
y = 32;
x = x + y;
print x;
```

Then run:

```bash
./compiler < myprogram.txt
llc out.ll -filetype=obj -relocation-model=pic -o out.o
clang -fPIE out.o -o myprog
./myprog
```

### Option C — Use the run script (easiest)

```bash
chmod +x run.sh
./run.sh myprogram.txt
```

This compiles and runs your program in one command.

---

## Examples

### Basic assignment and print

**Input:**
```
x = 42;
print x;
```

**Output:**
```
42
```

---

### Arithmetic with correct precedence

**Input:**
```
x = 1 + 5 * 27;
print x;
```

**Output:**
```
136
```
`*` binds tighter than `+`, so this evaluates as `1 + (5 * 27) = 136`.

---

### Parentheses to override precedence

**Input:**
```
x = (1 + 5) * 27;
print x;
```

**Output:**
```
162
```

---

### Multiple variables

**Input:**
```
x = 10;
y = 32;
x = x + y;
print x;
```

**Output:**
```
42
```

---

## How it works

```
Source code (.txt)
       ↓
   Flex (lexer.l)        → tokenises characters into NUM, VARIABLE, PRINT ...
       ↓
   Bison (parser.y)      → applies grammar rules, builds an AST
       ↓
   AST (ast.c / ast.h)   → tree of nodes representing the program
       ↓
   LLVM codegen          → walks the AST, emits LLVM IR (out.ll)
   (codegen.c)
       ↓
   llc + clang           → compiles IR to a native binary
       ↓
   ./myprog              → runs the program
```

---

## Clean build files

```bash
make clean
```

---

## Requirements

- Ubuntu 20.04 or later
- clang
- flex
- bison
- llvm-dev (LLVM 18 recommended)