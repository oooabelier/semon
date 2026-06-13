# Merged Technical Specification: Command-Line Electronic Circuit Simulator “semon”

## 1. General Project Requirements

### 1.1 Project Identity

- **Project name:** `semon`
- **Programming language:** C++ strictly compliant with the **C++11** standard.
- **Application type:** Cross-platform command-line interface application.
- **Core purpose:**  
  `semon` is an electronic digital circuit simulator and satisfiability-oriented modeling tool. It must support:
  - mathematical descriptions of bit-level systems;
  - synthesis of those descriptions into circuit topologies;
  - construction of larger circuits by interconnecting generated circuits and composite subcircuits;
  - unlimited-depth structural nesting;
  - bidirectional testing by locking signal values on arbitrary pins and propagating constraints forward or backward.

### 1.2 Target Platforms

The project must support the following operating systems and architectures:

#### Operating systems

- Windows using MSVC or MinGW
- Linux using GCC or Clang
- Android using Termux-compatible toolchains
- macOS using Clang
- iOS

#### Hardware architectures

- AMD64 / x86_64
- I386
- ARM32
- ARM64 / AArch64
- RISCV

### 1.3 CLI Usability Requirements

The interactive command-line shell must provide:

- native tab completion for:
  - commands;
  - file paths;
  - active variable names;
  - active mathematical model identifiers;
  - circuit identifiers;
  - pin, bus, gate, and subcircuit identifiers;
- persistent command history across sessions;
- ANSI color-coded output for signal states, for example:
  - green for logical high / `1`;
  - red for logical low / `0`;
  - yellow for floating, unknown, or unconstrained states;
- structured tabular and matrix-style output for introspection and debugging.

---

## 2. Project Directory and File Structure

### 2.1 Required Root Path

All project files must reside exactly under:

```text
<current_user_home_directory>/projects/cpp/semon
```

No alternative root path is allowed for the required project layout.

### 2.2 Required Directory Architecture

The project root must contain the following items exactly as named:

```text
readme.md
license.txt
include/
src/
test/
docs/
build/
.github/workflows/
```

The required meaning of each item is:

- `readme.md`  
  Architectural overview, build instructions, target matrix, and directory roadmap.

- `license.txt`  
  Full, unmodified text of the MIT License.

- `include/`  
  Public C++ header definitions using `.hpp` files.

- `src/`  
  C++ implementation files using `.cpp` files.

- `test/`  
  Unit tests, verification frameworks, fuzzers, cryptographic test harnesses, and benchmark suites.

- `docs/`  
  Plaintext technical documentation explaining mathematical assumptions, circuit topology rules, allocator design, and synthesis logic.

- `build/`  
  Build artifacts, intermediate files, target output directories, and the master `Makefile`.

- `.github/workflows/`  
  Strict automated CI/CD pipeline definitions.

### 2.3 Required Header Layer Partitioning

The `include/` directory must be partitioned by architectural layer:

```text
include/core/
include/math/
include/circuit/
include/persistence/
include/diagnostics/
```

Required responsibilities:

- `include/core/`
  - paged memory allocator;
  - global registries;
  - deterministic PRNG;
  - low-level configuration.

- `include/math/`
  - AST nodes;
  - ROBDD structures;
  - ESOP structures;
  - variables;
  - constants;
  - mathematical functions;
  - equation and inequality descriptions.

- `include/circuit/`
  - circuit topology objects;
  - direction-agnostic pins;
  - signal buses;
  - virtual 3-pin gates;
  - subcircuit instances;
  - net routing structures.

- `include/persistence/`
  - zero-heap streaming JSON readers and writers;
  - callback interfaces;
  - serialization and deserialization contracts.

- `include/diagnostics/`
  - assertion macros;
  - loop instrumentation;
  - watchdog counters;
  - global failure interface;
  - debug breakpoint helpers.

The `src/` directory must mirror the same conceptual structure.

---

## 3. Static Library and Code Isolation Requirements

### 3.1 Mandatory Static Core Library

All functional logic must be compiled into exactly one static library:

```text
libsemon_core.a
```

This library must contain all:

- memory management;
- mathematical parsing;
- AST, ROBDD, and ESOP logic;
- synthesis algorithms;
- circuit topology management;
- solver logic;
- persistence logic;
- diagnostics;
- optimization routines;
- allocator telemetry;
- PRNG logic;
- testing-support core APIs.

### 3.2 Thin Binary Requirement

The executable entry points must not contain core simulation logic.

Specifically:

- `src/main.cpp` must be only a thin CLI orchestration layer.
- `src/test_entry.cpp` must be only a thin testing harness entry point.

They may:

- initialize the application;
- run command loops;
- handle line-editing buffers;
- dispatch commands;
- invoke the test framework;
- call APIs from `libsemon_core.a`.

They must not implement:

- circuit algorithms;
- allocator internals;
- solver logic;
- synthesis logic;
- AST processing;
- serialization logic;
- topology models;
- mathematical evaluation algorithms.

---

## 4. Build System Requirements

### 4.1 Makefile Location

The project must be built using a standard GNU Makefile located inside:

```text
build/Makefile
```

All build commands must be executed from inside the `build/` directory.

### 4.2 Build Parameters

The Makefile must accept the following external human-readable variables:

```text
OS
ARCH
MODE
```

Examples:

```bash
make OS=Linux ARCH=AMD64 MODE=Debug
make OS=Windows ARCH=ARM64 MODE=Release
make OS=Android ARCH=RISCV MODE=Performance
```

### 4.3 Supported Parameter Values

#### OS values

| Full value | Output code |
|---|---|
| `Windows` | `w` |
| `Linux` | `l` |
| `macOS` | `m` |
| `Android` | `a` |
| `iOS` | `i` |

#### ARCH values

| Full value | Output code |
|---|---|
| `AMD64` | `x` |
| `I386` | `i` |
| `ARM32` | `a` |
| `ARM64` | `b` |
| `RISCV` | `r` |

#### MODE values

| Full value | Output code |
|---|---|
| `Debug` | `d` |
| `Performance` | `p` |
| `Release` | `r` |

The output subdirectory must be:

```text
build/[os-code][arch-code][mode-code]/
```

Example:

```bash
make OS=Linux ARCH=AMD64 MODE=Debug
```

must place all object files, static libraries, and final binaries inside:

```text
build/lxd/
```

Another example:

```bash
make OS=Linux ARCH=ARM64 MODE=Debug
```

must use:

```text
build/lbd/
```

### 4.4 Obfuscated Object Naming

Every intermediate object file must have an explicitly hardcoded, obfuscated, exactly three-character lowercase filename.

Example:

```text
src/core/PagedAllocator.cpp -> build/lxd/rzo.o
src/math/ExpressionParser.cpp -> build/lxd/arb.o
```

The mapping must be deterministic and explicitly declared in the Makefile.

### 4.5 Static Makefile Restrictions

The Makefile must be structurally predictable and declarative.

The following are strictly forbidden:

- `$(shell ...)`
- `$(eval ...)`
- `$(wildcard ...)`
- runtime macro iteration blocks;
- dynamic source discovery;
- dynamic symbol generation;
- imperative text loops;
- execution-time name masking.

Allowed mechanisms:

- explicit hardcoded compilation rules;
- conditional string assignments;
- simple `ifeq` / `else` logic;
- token concatenation such as `OBJ_$(MODE)`;
- explicitly declared object mappings.

### 4.6 Build Strictness

All targets must compile with:

```text
-std=c++11
-Wall
-Wextra
-Werror
```

The core must avoid C++ exception handling:

- no `throw`;
- no `try`;
- no `catch`.

Error states must be handled through:

- deterministic return codes;
- global diagnostic flags;
- assertions;
- controlled termination states;
- explicit failure objects stored in the allocator or diagnostic layer.

### 4.7 Static Analysis

The build pipeline must support integration with static analysis tools such as:

- `clang-tidy`;
- `cppcheck`.

Static analysis invocation must also follow the Makefile restrictions above.

---

## 5. Memory Architecture and Allocation Rules

## 5.1 Standard Heap and Container Restrictions

### 5.1.1 Heap Prohibition

During circuit building, mathematical model storage, topology construction, synthesis, persistence reconstruction, and simulation evaluation, persistent data must not be allocated using:

- `malloc`;
- `free`;
- global `new`;
- global `delete`;
- indirect heap-owning structures.

All long-lived simulator artifacts must be stored inside the custom paged allocator.

This includes:

- circuits;
- subcircuits;
- pins;
- gates;
- nets;
- buses;
- mathematical models;
- equations;
- inequalities;
- AST nodes;
- ROBDD nodes;
- ESOP nodes;
- constants;
- cryptographic round constants;
- user-defined functions;
- object names;
- metadata;
- configuration objects;
- diagnostic histories that must persist;
- active session state.

### 5.1.2 Standard Container Prohibition

C++ Standard Library containers must not be used for permanent modeling or runtime graph storage.

Forbidden for permanent simulator structures:

- `std::vector`;
- `std::list`;
- `std::deque`;
- `std::map`;
- `std::unordered_map`;
- `std::set`;
- `std::string` as persistent topology/name storage;
- any equivalent heap-owning standard container.

### 5.1.3 Limited Temporary Exceptions

The following are allowed only for short-lived local work:

- `std::string` for temporary command parsing or formatting;
- stack-allocated temporary buffers;
- stack-allocated evaluation variables;
- short-lived local parsing frames;
- small fixed stack arenas for streaming JSON token handling;
- temporary operational matrices used only during a simulation step.

These temporary objects must not own or manage permanent graph topology, mathematical structure, object names, or session state.

---

## 5.2 Native Paged Allocator

### 5.2.1 Native Page Mapping

All persistent objects must be allocated inside a custom low-level paged memory allocator.

The allocator must map objects directly onto flat native OS or hardware pages using the smallest available page granularity supported by the target platform.

Typical example:

```text
4096 bytes on x86_64 and ARM64
```

The allocator must not use:

- intermediate master pages;
- sub-page virtual pooling structures;
- generic heap-backed arenas;
- DOM-like temporary heap structures for persistence.

### 5.2.2 Page Homogeneity

Each physical page must contain objects of exactly one structural type.

Examples:

- one page only for `Pin`;
- one page only for `Virtual3PinGate`;
- one page only for `ExpressionNode`;
- one page only for medium constant blocks;
- one page only for a specific metadata type.

Mixing unrelated structure types on one page is forbidden.

### 5.2.3 Parent-Container Isolation

If a parent object owns a collection of child elements, every page storing those child elements must include metadata containing a raw pointer back to that exact parent container.

Example:

- a circuit owns gates;
- pages containing those gates must store a pointer to that circuit.

A page must never mix elements belonging to different parent containers.

Therefore:

- gates from two different circuits cannot share a page;
- pins from two different parent circuit instances cannot share a page;
- AST nodes from different mathematical models cannot share a page unless they belong to the same explicitly designated parent container.

---

## 5.3 Page Chaining and Diagnostics

### 5.3.1 Doubly Linked Page Chains

If one page is insufficient for all objects of a given type belonging to a given parent container, additional native pages must be linked into a dynamic doubly linked chain.

Each page header must contain:

- pointer to previous page: `prev`;
- pointer to next page: `next`;
- pointer to parent container;
- object type identifier;
- allocation statistics;
- active/dead object counters.

### 5.3.2 Page Telemetry

The allocator must track page-level diagnostics:

- bytes allocated;
- bytes unused;
- active objects;
- released/dead objects;
- free slots;
- fragmentation ratio;
- page chain length;
- parent container pointer;
- object type;
- page address.

### 5.3.3 CLI Visualization

The CLI must expose commands to inspect and visualize allocator state, including:

- page chains;
- memory layout per page;
- active vs released objects;
- fragmentation;
- parent ownership;
- raw page addresses;
- object offsets;
- type-specific pools.

Output must be text-based and suitable for real-time debugging.

---

## 5.4 Release Marking, Defragmentation, and Compaction

### 5.4.1 Released-State Marking

Objects no longer directly used by simulation logic may be marked with a lightweight `released` state flag.

This avoids immediate compaction after every deletion or simulation step.

Released objects may remain discoverable for allocator-level diagnostics until compaction occurs.

### 5.4.2 Automatic Compaction Triggers

The allocator must support automatic compaction using both required monitoring rules:

1. **Memory-scarcity trigger**  
   Automatic compaction must occur when the number of completely free native pages drops below the configured `1/N` fraction of the total potentially available usable page capacity.

2. **Arena-efficiency trigger**  
   Let:
   - `U` be the number of fully or partially used pages;
   - `F` be the number of completely free reserved pages in the active allocator context.

   The allocator must also monitor:

   ```text
   F >= U / N
   ```

   and perform the required arena maintenance/defragmentation action when this condition is met.

The divisor `N` must be a static configuration value with default value `4`.

### 5.4.3 Manual Compaction

The CLI and internal structural events must be able to force compaction on demand.

### 5.4.4 Type-Specific Compaction

Generic garbage collectors, reflection-based scanners, and universal pointer-tracing compactors are forbidden.

Each structure type must implement its own hardcoded compaction algorithm.

Each compactor must know:

- exact object size;
- exact field offsets;
- exact pointer fields;
- exact back-reference fields;
- parent container mutation points;
- page chain links;
- alignment rules;
- reference counts.

Compaction must use linear block-stride operations and direct pointer arithmetic.

### 5.4.5 Pointer Remapping

During compaction, internal raw pointers must be directly patched and remapped.

This applies to:

- child pointers;
- parent pointers;
- gate-to-pin links;
- pin-to-net links;
- circuit-to-subcircuit links;
- AST child links;
- BDD low/high branch links;
- constant container links;
- page chain links.

The purpose is to avoid the overhead of generic garbage collection, tracing, sweeping, or runtime reference-table scanning.

---

## 6. Global Configuration and Coding Style

## 6.1 Java-Like Encapsulation

The codebase must follow strict object-oriented encapsulation.

Internal fields must not be freely mutated from outside.

Access to internal state must be provided through public constant getters, for example:

```cpp
uint32_t getVariableUid() const;
const Pin* getPinA() const;
const Circuit* getParentCircuit() const;
```

Standalone public setters are prohibited on layout-defining structures.

Mutation of internal raw references is allowed only during:

1. internal compilation and synthesis;
2. active solving and propagation;
3. stream-driven deserialization;
4. allocator compaction and pointer remapping.

## 6.2 Magic Literal Ban

Execution algorithms and conditional logic must not contain unexplained inline numeric masks, string keys, status codes, or lookup values.

Required constants must be declared as:

- `static constexpr` fields inside the relevant class; or
- values inside a single global configuration class.

Example:

```cpp
class SystemSettings {
public:
    SystemSettings() = delete;

    static constexpr size_t NATIVE_PAGE_SIZE = 4096;
    static constexpr uint32_t DEFRAGMENTATION_DIVISOR = 4;
    static constexpr uint32_t MAXIMUM_BIT_WIDTH = 65536;
    static constexpr uint64_t MAX_LOOP_CYCLES = 10000000;
};
```

## 6.3 Assertions and Design by Contract

Every internal framework function must begin with explicit protective assertion or contract checks.

These checks must validate:

- non-null pointer arguments;
- valid object ownership;
- valid page membership;
- valid array or bit indices;
- valid enum ranges;
- valid size bounds;
- valid parent-container relationships.

## 6.4 Debug Diagnostics

In Debug builds, identified by target directory mode code `d`, the engine must retain specialized diagnostic structures.

### 6.4.1 Global Error Flag

The core must define exactly one global boolean:

```cpp
g_errorFlag
```

When a safety boundary, contract condition, allocator violation, math contradiction, or structural invariant fails, `g_errorFlag` must be set to `true`.

The diagnostic system must preserve:

- component reference;
- page address;
- object offset;
- internal error code;
- cycle count;
- optional textual diagnostic identifier.

### 6.4.2 Loop Instrumentation

Every internal loop body in:

- simulation;
- SAT solving;
- propagation;
- synthesis traversal;
- compaction;
- persistence parsing;

must invoke a specialized loop instrumentation macro at the beginning of the loop body.

In Release builds, this macro must compile to a no-op.

In Debug builds, it must register execution cycles through a central function suitable for hardware breakpoint placement.

If a loop exceeds:

```cpp
SystemSettings::MAX_LOOP_CYCLES
```

the global error flag must be set and execution must abort or return through a controlled diagnostic path.

---

## 7. Deterministic PRNG

The core must include a deterministic pseudo-random number generator implemented from scratch.

Allowed examples:

- LCG;
- Xoshiro-style generator;
- another explicitly implemented deterministic generator.

The PRNG must:

- accept a 64-bit seed;
- expose the current seed/state for diagnostics;
- produce identical sequences on all supported platforms for the same seed;
- be used for fuzz testing, randomized constraint selection, and stochastic exploration where needed.

No platform-dependent random generator may be used for reproducibility-critical operations.

---

## 8. Mathematical Model Layer

## 8.1 Separation from Circuit Topology

The mathematical description layer and circuit topology layer are separate conceptual layers.

The mathematical layer stores:

- variables;
- constants;
- equations;
- inequalities;
- user-defined functions;
- built-in function references;
- ASTs;
- ROBDDs;
- ESOP structures;
- symbolic constraints.

The mathematical layer must not execute logical or arithmetic calculations as a runtime engine.

All actual constraint evaluation, propagation, and signal-state processing must occur in the circuit topology layer through virtual gates and solver routines.

The only allowed overlap is optional metadata mapping a generated physical pin to a mathematical variable bit index during synthesis.

## 8.2 Mathematical Object Behavior

A mathematical description object is a static or editable blueprint.

It may be modified by the user, but it must not directly calculate runtime signal values.

It must support adding, editing, and deleting:

- variables;
- constants;
- equations;
- inequalities;
- user-defined functions;
- cryptographic built-in invocations;
- expression trees.

Changes to a mathematical model must not automatically rebuild circuits.

Circuit regeneration must occur only after an explicit CLI compilation command.

---

## 9. Variables, Constants, and Bit Widths

## 9.1 Variables

Each mathematical variable represents an unsigned integer bit-vector.

The bit width:

- is defined at initialization;
- must be a natural number;
- may scale up to exactly:

```text
2^16 bits = 65,536 bits
```

No internal primitive may impose a 32-bit or 64-bit maximum on variable width.

Because the mathematical layer does not evaluate values, non-constant variables must not store runtime values. They exist as symbolic identifiers and are mapped to pins during circuit synthesis.

## 9.2 Constants

Constants must be stored according to size class.

### 9.2.1 Small Constants

A small constant has size less than or equal to the native machine word size:

```text
sizeof(uintptr_t)
```

Its literal bits must be stored directly inside the constant tracking object, replacing the data-pointer field.

### 9.2.2 Medium Constants

A medium constant is larger than a native machine word and up to exactly half of the native page size.

Example on a 4096-byte page system:

```text
> 8 bytes and <= 2048 bytes
```

Medium constants must be allocated inside homogeneous native pages and must include a back-reference to their parent mathematical model.

### 9.2.3 Large Constants

A large constant is larger than half of the native page size and up to the maximum supported bit width.

Large constants must be distributed across linked native page chains and must include a back-reference to their parent constant container.

Large constants must not require a single contiguous virtual allocation.

---

## 10. Mathematical Operations

The AST syntax engine and compiler must support standard boolean logic and the following advanced operations.

### 10.1 Relations

Supported relations include:

- equality: `==`;
- inequality: `<`;
- inequality: `>`;
- inequality: `<=`;
- inequality: `>=`.

Both left-hand side and right-hand side expressions must be equally powerful.

Each side may contain:

- variables;
- constants;
- slices;
- shifts;
- modulo operations;
- concatenations;
- deeply nested expressions;
- user-defined function calls;
- built-in cryptographic function calls.

The parser must not require either side to be simplified to a single variable or constant.

### 10.2 Arithmetic and Bitwise Operations

Required operations include:

- modulo / remainder: `%`;
- bitwise left shift: `<<`;
- bitwise right shift: `>>`;
- logical shifts;
- arithmetic shifts where meaningful;
- bit range extraction / slicing, for example:

```text
X[4:8]
```

- bit concatenation, for example:

```text
A || B || C
```

The implementation must support arbitrary bit widths up to 65,536 bits.

---

## 11. User-Defined and Built-In Functions

## 11.1 User-Defined Functions

Users must be able to define custom mathematical functions containing:

- expressions;
- equations;
- local variables;
- local argument remappings;
- nested calls to other user-defined functions.

User-defined functions must support arbitrary nesting depth.

## 11.2 Flattening During Synthesis

When a mathematical model is compiled into a basic circuit, all functional abstractions must be dissolved.

The generated basic circuit must be a flat network of:

- `Virtual3PinGate` objects;
- pins;
- buses;
- routing structures;
- parity constraints.

No local function frames, hidden function modules, or hierarchical function boundary blocks may remain in the generated basic circuit.

## 11.3 Built-In Cryptographic Functions

Cryptographic structures must be treated as native built-in mathematical functions.

They must bypass text-based user definition and compile directly into optimized flat gate constraints.

Required built-ins include:

### Hash functions

- `MD5(X)`
- `SHA256(X)`
- `SHA512(X)`
- `SHA3(X)` / Keccak-f

The compiler must expand these into deterministic static constraint graphs containing the relevant round constants, message schedules, state transformations, and bit operations.

Examples:

- MD5: `F`, `G`, `H`, `I`;
- SHA-256: `Σ0`, `Σ1`, `Ch`, `Maj`;
- SHA-3: `θ`, `ρ`, `π`, `χ`, `ι`.

### AES block cipher primitive

The AES builder must include constraints for:

- ByteSub / S-Box transformations;
- GF arithmetic;
- ShiftRows;
- MixColumns;
- KeyExpansion.

### Modular asymmetric operations

Required high-level modular blocks:

```text
MODEXP(Base, Exp, Mod)
ECC_ADD(P1, P2, CurveParams)
ECC_MUL(Scalar, P1, CurveParams)
ECDSA_VERIFY(MsgHash, Signature, PublicKey, CurveParams)
```

`MODEXP` must support scalable dimensions up to 65,536 bits.

ECC functions must support NIST prime curves such as:

- P-256;
- P-384;
- P-521.

`ECDSA_VERIFY` must map the full verification loop into structural constraints, including:

- modular inverse;
- scalar multiplication;
- point addition;
- signature equation checks.

---

## 12. Synthesis Architecture: AST, ROBDD, and ESOP

## 12.1 Required Core Representations

The mathematical synthesis layer must process boolean systems through:

1. ASTs for original expressions and equations;
2. ROBDDs for canonical logic representation and control-heavy expressions;
3. ESOP / Reed-Muller / Zhegalkin forms for XOR-heavy and cryptographic logic.

## 12.2 ROBDD Pipeline

The compiler must transform standard relational constraints, multi-terminal operators, and conditional subexpressions into ROBDDs.

The ROBDD pipeline must perform:

- aggressive common subexpression elimination;
- constant folding;
- canonical node sharing;
- reduction before physical netlist emission.

ROBDD node semantics must map naturally to 2-to-1 multiplexer-style behavior during gate generation.

## 12.3 ESOP Pipeline

The compiler must synthesize cryptographic and linear/XOR-heavy subfunctions into ESOP structures.

This is required for:

- XOR cascades;
- rotations;
- shifts;
- ARX-style functions;
- hash rounds;
- AES-like mixing;
- bitwise additions when suitable.

The purpose is to produce dense virtual 3-pin gate arrangements rather than exponential naive gate expansion.

## 12.4 AST and ROBDD Node Allocation

All AST and BDD nodes must be allocated inside homogeneous pages of the custom allocator.

Standard containers and heap-allocated trees are forbidden.

The required low-level node profile is:

```cpp
enum class MathNodeType : uint8_t {
    VARIABLE,
    CONSTANT,
    OPERATION,
    BDD_CHOICE
};

struct ExpressionNode {
    ExpressionNode* left;
    ExpressionNode* right;
    void* parent_container;
    uint32_t variable_uid;
    MathNodeType node_type;
    uint8_t flags;
};
```

The implementation may extend this layout only if it preserves the required fields, raw-pointer structure, compact memory profile, and allocator restrictions.

---

## 13. Circuit Topology Layer

## 13.1 Circuit Objects

A circuit object stores concrete structural topology, including:

- pins;
- gates;
- buses;
- nets;
- nested subcircuit instances;
- boundary pin groups;
- routing networks;
- locked signal states;
- constraint checker blocks.

A circuit must not natively store mathematical equations.

A basic circuit is produced by explicitly compiling a mathematical description.

A composite circuit is built by structurally instantiating and connecting existing circuits.

## 13.2 Unlimited Structural Nesting

Composite circuits may contain:

- generated basic circuits;
- other composite circuits;
- multiple instances of the same blueprint.

There must be no artificial limit on nesting depth.

Each circuit instance must have independent:

- spatial state;
- boundary routing;
- pin mappings;
- signal locks;
- interconnects.

## 13.3 Multiple Instantiation

A circuit blueprint may be instantiated multiple times inside the same parent circuit.

Each instance must be isolated and independently connectable to different wires, buses, or neighboring components.

## 13.4 Pin Groups and Signal Buses

Pins may be grouped into ordered pin groups or buses.

Typical use case:

- grouping the bits of one mathematical variable in the original bit order.

These groups must simplify subcircuit interconnection without introducing directional input/output semantics.

## 13.5 Direction-Agnostic Pins

Pins must not be hardcoded as inputs or outputs.

Every pin is an undirected physical constraint endpoint.

Propagation direction depends only on which pins or buses are locked during solving.

---

## 14. Virtual 3-Pin Gate Model

## 14.1 Single Normalized Gate Basis

Every generated circuit must normalize its internal logic into one primitive:

```text
Virtual3PinGate
```

This gate is:

- symmetric;
- direction-agnostic;
- local-equilibrium based;
- configured by an 8-bit truth table;
- used as a balance checker rather than a traditional directional signal driver.

Minor separate gates such as buffers, wires, and inverters must not exist as independent hardware types.

## 14.2 Truth-Byte Semantics

A `Virtual3PinGate` has up to three pins:

- Pin A;
- Pin B;
- Pin C.

The gate uses an 8-bit configuration byte called the Truth-Byte.

Each bit index corresponds to a 3-bit state:

```text
0bCBA
```

Mapping:

- Pin A is bit 0 of the index;
- Pin B is bit 1 of the index;
- Pin C is bit 2 of the index.

A Truth-Byte bit value means:

- `1`: this pin-state combination is valid;
- `0`: this pin-state combination violates the gate constraint.

## 14.3 Required Encoding Examples

### AND

For:

```text
C = A AND B
```

valid states are:

```text
0b000
0b001
0b010
0b111
```

Truth-Byte:

```text
0b10000111 = 0x87
```

### XOR

For:

```text
C = A XOR B
```

valid states are:

```text
0b000
0b011
0b101
0b110
```

Truth-Byte:

```text
0b01101001 = 0x69
```

## 14.4 Implication and Parity Constraints

If a direct implication or synchronous relationship exists between wires, the gate may be instantiated with a missing third pin.

In that case:

- the third pin pointer must be strictly `nullptr`;
- the gate must still retain the full 8-bit truth table layout;
- all three index bits remain part of the conceptual lookup mask.

If, during synthesis, the missing third signal becomes known, the third pin must be rebound to the corresponding bus or wire.

### Even parity

For direct / in-phase synchronization, the Truth-Byte must allow only states where the number of high pins across A, B, and C is even.

### Odd parity

For inverted / out-of-phase synchronization, the Truth-Byte must allow only states where the number of high pins across A, B, and C is odd.

All invalid parity combinations must be excluded from the truth table.

The implementation must define explicit constants for all required parity encodings, including implication-direction-specific encodings required by the synthesis mapping.

## 14.5 Zero-Cost Inversion by Truth-Byte Permutation

The `Virtual3PinGate` class must not contain runtime per-pin inversion flags used during propagation.

Logical inversion must be resolved by permuting the Truth-Byte during compilation, synthesis, or gate configuration.

Required permutations:

### Invert Pin A

```text
TruthByte = ((TruthByte & 0xAA) >> 1) | ((TruthByte & 0x55) << 1)
```

### Invert Pin B

```text
TruthByte = ((TruthByte & 0xCC) >> 2) | ((TruthByte & 0x33) << 2)
```

### Invert Pin C

```text
TruthByte = (TruthByte >> 4) | (TruthByte << 4)
```

These formulas must be represented using named `static constexpr` masks rather than magic literals inside algorithms.

## 14.6 Buffer and Inverter Emulation

Standalone buffer, wire, and inverter components are forbidden.

A buffer must be emulated using a `Virtual3PinGate` configured so that:

```text
A = B
```

The third pin may be:

- left open;
- bound to a dummy system line;
- bound to a known synchronous signal if synthesis determines one.

An inverter must be emulated using the same structure, but with the Truth-Byte permuted so the stable constraint is:

```text
A != B
```

No separate inverter component type may be introduced.

## 14.7 Boundary Truth-Byte Values

The simulator must recognize and optimize two special Truth-Byte values.

### `0xFF` — Universal Freedom

Represents complete logical freedom.

All pin combinations are valid.

Uses:

- soft deletion;
- bypass routing;
- raw page initialization;
- passive floating junctions;
- disabled gates.

Propagation must skip `0xFF` gates to avoid unnecessary work.

### `0x00` — Universal Contradiction

Represents an impossible state.

No pin combination is valid.

Uses:

- early conflict detection;
- failed SAT branches;
- rollback triggering;
- contradiction localization.

When encountered during solving, it must immediately signal contradiction and invoke the solver’s controlled rollback or backtracking path.

---

## 15. SAT Solver and Propagation Engine

## 15.1 Custom Solver Requirement

The satisfiability and propagation engine must be implemented entirely from scratch.

External SAT, SMT, or constraint solver dependencies are forbidden, including but not limited to:

- Z3;
- MiniSat;
- CryptoMiniSat;
- Boolector;
- CVC;
- external BDD packages.

## 15.2 Bidirectional Propagation

The circuit solver must support bidirectional constraint propagation.

It must be able to:

1. determine outputs from locked inputs;
2. determine inputs from locked outputs;
3. propagate partial constraints from any arbitrary locked pins;
4. detect contradictions;
5. leave unresolved wires floating when no local deduction is possible.

The solver must not assume fixed input/output directions.

## 15.3 Partial and Non-Full Propagation

For cryptographic reverse modeling and other NP-hard cases, the solver must support graceful early termination.

If a backward hash preimage search, differential cryptographic attack, or modular arithmetic inversion becomes structurally intractable within configured limits, the engine may stop safely.

Unresolved wires must remain floating or unconstrained.

The solver must not hang indefinitely.

## 15.4 Watchdog

All iterative solving loops must be monitored.

If execution exceeds:

```text
10^7 iterations
```

or:

```cpp
SystemSettings::MAX_LOOP_CYCLES
```

the engine must abort through controlled diagnostics and set `g_errorFlag` in Debug mode.

## 15.5 Propagation Callbacks

The propagation engine must expose interception hooks.

At minimum, it must support the following callback signature:

```cpp
bool (*propagation_hook)(
    uint64_t current_cycle,
    size_t active_gates_count,
    void* userdata
);
```

This callback must be called at major propagation layer boundaries or iteration ticks.

The engine must pass:

- absolute cycle count;
- count of gates still unstable or queued;
- the caller-provided `userdata`.

If the callback returns `false`, the solver must:

- immediately halt;
- roll back volatile tentative pin states to the last stabilized checkpoint where required;
- exit safely;
- return control to the caller.

In addition, the solver must provide granular diagnostic interception at the start of individual implication steps, variable assignments, or gate evaluations. That diagnostic callback must expose enough state to inspect:

- implication depth;
- target gate identifier;
- assigned bit masks;
- active pin or bus;
- current diagnostic cycle;
- `userdata`.

A no-op callback implementation may be supplied when a caller does not need interruption, but the propagation engine API must support callback-based tracking.

---

## 16. Testing Requirements

## 16.1 Test Harness Structure

Tests must be stored under:

```text
test/
```

and driven through the thin entry point:

```text
src/test_entry.cpp
```

Core test logic must live in `libsemon_core.a` or test modules that call the library.

## 16.2 Directional Verification Strategy

To avoid deadlocks during verification, tests must split propagation strategy by domain.

### Mathematical and compiler verification

Must use backward propagation.

The test framework must lock output pins or resulting nodes and verify that the compiled topology can synthesize valid input combinations without illegal contradictions.

### Cryptographic primitive verification

Must use forward propagation only.

The test harness must inject known input vectors and propagate forward to compute outputs.

Backward cryptographic propagation must not be used in fuzzing paths where it could create deep recursive search trees or solver deadlocks.

## 16.3 Fuzzing

Fuzz tests must use the deterministic PRNG.

Every fuzz run must record enough information to reproduce failures:

- seed;
- target platform;
- build mode;
- model identifier;
- operation sequence;
- signal injection vector;
- active circuit state.

---

## 17. Persistence and JSON Streaming

## 17.1 Universal Persistence Requirement

The application must support saving and loading all major object layers.

Persistence must not be limited to mathematical descriptions.

The persistence subsystem must serialize and reconstruct:

1. mathematical descriptions;
2. variables;
3. constants;
4. equations;
5. inequalities;
6. user-defined functions;
7. AST structures;
8. ROBDD structures;
9. ESOP matrices;
10. circuit topology graphs;
11. pins;
12. buses;
13. virtual gates;
14. nested subcircuit configurations;
15. wire routes;
16. locked signal vectors;
17. boundary pin states;
18. active session state;
19. allocator page chains;
20. diagnostic metadata;
21. configuration settings;
22. PRNG state.

## 17.2 Streaming JSON Only

Persistence must be stream-oriented JSON.

The system must not parse JSON into a heap-backed DOM tree, token array, object map, or intermediate in-memory hierarchy.

Deserialization must read sequentially and instantiate objects directly into the custom paged allocator.

Serialization must stream data directly out through callbacks.

## 17.3 Callback Interfaces

The persistence layer must support low-level reader and writer callback contracts.

### Writer callback

The system must support the boolean writer signature:

```cpp
bool (*write_fn)(const char* block, size_t length, void* userdata);
```

It must also support or adapt the size-returning writer form:

```cpp
size_t (*write_func)(const char* data, size_t size, void* userdata);
```

The implementation must clearly define how partial writes or failures are converted into deterministic error states.

### Reader callback

The required reader signature is:

```cpp
size_t (*read_fn)(char* buffer, size_t max_length, void* userdata);
```

The `userdata` raw pointer must be passed transitively through all persistence frames.

This allows binding to:

- files;
- sockets;
- memory streams;
- custom platform I/O handles;

without violating allocator isolation.

## 17.4 Reconstruction Integrity

Loading an exported state must produce a deterministic reconstruction of the simulator state.

The reconstructed state must preserve:

- object identity relationships;
- parent back-references;
- page-chain organization;
- constant size classes;
- active locked signals;
- topology wiring;
- gate Truth-Bytes;
- diagnostic state;
- PRNG state.

After save and load, subsequent solver operations must behave identically to operations before serialization.

---

## 18. Command-Line Interface Functional Requirements

The CLI must expose commands for the following domains.

### 18.1 Context Control

The user must be able to select and switch the active editing context between:

- mathematical model;
- circuit topology;
- subcircuit instance;
- pin group;
- gate;
- bus;
- allocator pool;
- active session state.

### 18.2 Mathematical Editing

The CLI must allow:

- creating variables;
- deleting variables;
- defining constants;
- adding equations;
- modifying equations;
- deleting equations;
- adding inequalities;
- modifying inequalities;
- deleting inequalities;
- registering cryptographic built-in calls;
- defining user functions;
- editing user functions;
- deleting user functions.

### 18.3 Explicit Compilation

The CLI must provide commands to compile an active mathematical description into a flat basic circuit.

Compilation must never occur implicitly after mathematical edits.

### 18.4 Topology and Nesting Control

The CLI must allow:

- creating circuit blueprints;
- instantiating circuits inside parent circuits;
- instantiating the same blueprint multiple times;
- connecting pins and buses;
- browsing nested subcircuits;
- grouping pins into buses or harnesses;
- inspecting boundary pins.

### 18.5 Signal Injection

The CLI must allow users to force and lock signal values on:

- individual pins;
- pin ranges;
- buses;
- variable-mapped pin groups;
- nested subcircuit boundary pins.

Supported locked values:

- `0`;
- `1`.

Unspecified pins remain floating or unconstrained.

### 18.6 Simulation Control

The CLI must allow:

- starting propagation;
- stopping propagation through callback-controlled interruption;
- displaying stabilized states;
- showing floating states;
- showing contradictions;
- printing variable bit-state tables across nested structures.

### 18.7 Object Introspection

The CLI must provide commands to list and inspect:

- mathematical models;
- variables;
- constants;
- equations;
- AST nodes;
- ROBDD nodes;
- ESOP structures;
- circuits;
- subcircuits;
- pins;
- gates;
- buses;
- nets;
- page chains;
- allocator pools;
- active object cursors.

### 18.8 Pin Grid Matrix

The CLI must provide a structured Pin Grid Matrix view that allows:

- browsing nested pins step by step;
- inspecting raw node states;
- mapping pins back to allocator pages;
- visualizing wire interconnect chains;
- viewing gate Truth-Bytes;
- showing locked/floating/conflicting states.

### 18.9 Persistence Commands

The CLI must expose save and load commands using the streaming callback persistence subsystem.

The commands must support complete application state persistence, not only mathematical model persistence.

### 18.10 Runtime Statistics

The CLI must display live statistics, including:

- page utilization;
- free page count;
- active page count;
- fragmentation ratio;
- active implication count;
- active solver queue length;
- PRNG seed;
- solving duration;
- loop counter state;
- diagnostic error state.

---

## 19. Required Behavioral Summary

The final implementation of `semon` must satisfy all of the following high-level rules:

1. It is a strict C++11 cross-platform CLI application.
2. All core logic is isolated in `libsemon_core.a`.
3. Persistent simulator data is stored only in the custom native paged allocator.
4. Standard containers are forbidden for permanent topology, model, AST, BDD, runtime, and metadata structures.
5. Mathematical descriptions and circuit topologies are separate layers.
6. Mathematical models describe constraints but do not execute runtime calculations.
7. Circuits execute constraints through direction-agnostic virtual 3-pin gates.
8. Every generated basic circuit is flattened into `Virtual3PinGate` structures.
9. Composite circuits are built structurally and may nest without depth limits.
10. The solver is custom-built, bidirectional, interruptible, and watchdog-protected.
11. Cryptographic primitives are native built-in functions compiled into optimized structural constraints.
12. Persistence is universal, JSON-based, streaming, callback-driven, and zero-heap.
13. Debug builds must preserve strong diagnostics, loop tracking, and `g_errorFlag`.
14. The Makefile must be deterministic, static, and use explicit obfuscated object mappings.
15. The CLI must provide editing, compilation, simulation, persistence, introspection, memory visualization, and colored interactive usability features.