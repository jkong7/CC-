# CC-

A compiler for C-, a small C-like language, written as a chain of seven compilers in C++. Each stage lowers one intermediate language into the next until the last one emits x86-64 assembly. It started as the Northwestern CS 322 compiler sequence (L1, L2, L3 and IR) and grew from there into a full front end.

```
 .cm      .b       .a      .IR      .L3      .L2      .L1       .S
 C-  ──▶  LB  ──▶  LA  ──▶  IR  ──▶  L3  ──▶  L2  ──▶  L1  ──▶ x86-64 ──▶ a.out
```

| Stage | What it handles |
| --- | --- |
| CM | C- source: expressions with C precedence and every C integer operator, `&&`/`\|\|` short circuit, `?:`, `if`/`else`, `while`, `do`/`while`, `for`, `int` and `int[]...[]` arrays, type checking with line:column diagnostics |
| LB | nested scopes with shadowing, `if`/`while` on comparisons, `goto`, `continue`, `break` |
| LA | plain names and unencoded integers, null and bounds checks on every array access that report the source line, basic block formation |
| IR | typed variables, basic blocks, multi-dimensional array and tuple addressing, trace-based block linearization |
| L3 | instruction selection by tree tiling after merging trees within each context |
| L2 | liveness analysis, interference graph, graph coloring register allocation and spilling |
| L1 | x86-64 code generation for the calling convention used by every stage above |

Values follow the CS 322 representation: integers are tagged as `2n + 1` and pointers are left untagged, so the runtime can print nested arrays without type information.

## A C- program

```c
int[] sieve(int n) {
  int[] composite = new int[n + 1];
  for (int i = 2; i * i <= n; i++) {
    if (composite[i] == 0) {
      for (int j = i * i; j <= n; j += i) composite[j] = 1;
    }
  }
  return composite;
}

void main() {
  int[] c = sieve(100);
  int found = 0;
  for (int i = 2; i <= 100; i++) {
    if (!c[i]) found++;
  }
  print(found);
}
```

Builtins are `print(x)`, `input()` and `length(a, dim)`. None of the lower languages can divide, so `/` and `%` come from a small prelude written in C- (shift and subtract long division with C's truncating semantics) that is compiled in only when a program uses them. Dividing by zero gives 0. `|`, `^` and `~` lower to identities over `+`, `-` and `&`. Indexing past the end of an array stops the program with the position, the array length and the line.

## Optimizations

`-O1` is the default and `-O0` turns everything below off. Every test runs at both levels.

| Stage | Pass |
| --- | --- |
| LA | branch directly on a comparison instead of re-encoding it, compute sums and products into fresh temporaries so the tag adjustments can cancel |
| IR | global constant propagation (including function values, so indirect calls become direct), copy propagation, algebraic simplification, constant offset folding, null check elimination, liveness-based dead code elimination, branch folding, jump threading, unreachable block removal and block merging, iterated to a fixed point |
| IR codegen | array addressing that reads only the lengths it needs and folds constant indexes and dimensions |
| L3 | two-address arithmetic straight into the destination, compare-and-branch as one `cjump`, negated comparisons flipped, constant offsets folded into `mem`, callee-saved registers moved into variables so values live across calls can use them |
| L2 | save variables prefer their own register and are spilled before real values |
| L1 | peephole pass for self moves, overwritten moves and jumps to the next instruction |

For `total += i * 3` inside a `for` loop, the loop body at `-O1` is one move and three arithmetic instructions plus the loop increment and one `cmp`/`jge` pair. At `-O0` the same body is about thirty instructions.

Static x86-64 instruction counts for the programs in `bench/` (`scripts/bench`):

| Program | -O0 | -O1 |
| --- | ---: | ---: |
| fib | 91 | 59 |
| matmul | 1133 | 472 |
| sieve | 238 | 88 |
| sort | 828 | 341 |

## Building and running

The compilers build on any platform with a C++17 compiler. Linking and running programs needs an x86-64 Linux machine, so on anything else `scripts/dev` runs a command inside a `linux/amd64` container.

```sh
make                                          # fetches PEGTL into lib/ on first run
scripts/dev make test                         # build and run every test in the container
scripts/dev scripts/compile -o sieve tests/CM/primes.cm
scripts/dev ./sieve
```

`scripts/compile` accepts a source file from any stage (`.cm`, `.b`, `.a`, `.IR`, `.L3`, `.L2`, `.L1`, `.S`) and lowers it the rest of the way. `-e IR` stops after a given stage and `-k` keeps every intermediate file next to the output, which is the easiest way to see what each stage does.

## Tests

`tests/<stage>/` holds programs for every language with their expected output (`.out`), optional standard input (`.in`), and for programs that must not compile, the expected diagnostics (`.err`). `make test` or `scripts/test <filter>` runs them end to end. CI runs the whole suite on every push.

## Layout

```
CM/ LB/ LA/ IR/ L3/ L2/ L1/   one compiler per stage, each with src/
runtime/                      print, allocate, input and the array error handlers
scripts/                      compile driver, test runner, benchmarks, Docker wrapper, PEGTL bootstrap
bench/                        C- programs used to compare optimization levels
tests/                        end-to-end programs per stage
```
