# semon

`semon` is a C++11 command-line electronic digital circuit simulator and satisfiability-oriented modeling tool. This repository implements the required project layout, static core library, deterministic allocator, mathematical AST layer, virtual 3-pin gate topology, bidirectional propagation solver, streaming JSON persistence callbacks, deterministic PRNG, CLI shell, and self-test harness.

## Build

All build commands must run from `build/`:

```bash
cd ~/projects/cpp/semon/build
make OS=Linux ARCH=AMD64 MODE=Debug
```

Outputs are placed in the target directory derived from the requested platform:

- `build/lxd/` for Linux / AMD64 / Debug
- `build/lxp/` for Linux / AMD64 / Performance
- `build/lxr/` for Linux / AMD64 / Release

The static core library is:

```text
build/lxd/libsemon_core.a
```

The thin CLI binary is:

```text
build/lxd/semon
```

The thin test binary is:

```text
build/lxd/semon_test
```

Run tests:

```bash
./lxd/semon_test
```

Run the CLI non-interactively:

```bash
./lxd/semon "init; var A 2; equation A == A; compile; solve; stats"
```

## Target matrix

The Makefile accepts human-readable variables:

```bash
make OS=Windows ARCH=ARM64 MODE=Release
make OS=Android ARCH=RISCV MODE=Performance
make OS=macOS ARCH=AMD64 MODE=Debug
```

Supported OS values: `Windows`, `Linux`, `macOS`, `Android`, `iOS`.

Supported architecture values: `AMD64`, `I386`, `ARM32`, `ARM64`, `RISCV`.

Supported mode values: `Debug`, `Performance`, `Release`.

## Directory roadmap

```text
include/core/          allocator, PRNG, registries, shell support, tests
include/math/          AST, math model, parser, synthesis interface
include/circuit/       circuit topology, pins, buses, virtual gates, solver
include/persistence/   streaming JSON callback reader/writer
include/diagnostics/   global error flag, loop instrumentation, diagnostics
src/                   mirror of include layers
test/                  test documentation and future harness extensions
docs/                  plaintext design notes
build/Makefile         deterministic static Makefile with obfuscated object names
.github/workflows/     CI workflow
```

## Core design notes

- Persistent simulator objects are allocated in `core::PagedAllocator`.
- Standard containers are not used for permanent topology, AST, solver, or allocator structures.
- Mathematical models are blueprints and do not execute runtime signal values.
- Circuit topology executes constraints through direction-agnostic `Virtual3PinGate` objects.
- The solver is custom, interruptible through callbacks, and protected by `SystemSettings::MAX_LOOP_CYCLES`.
- Debug instrumentation is centralized through `Diagnostics` and `g_errorFlag`.
