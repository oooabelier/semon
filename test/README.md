# semon test directory

The executable self-test harness is built as `build/lxd/semon_test` in Debug mode.

Run:

```bash
cd build
make OS=Linux ARCH=AMD64 MODE=Debug
./lxd/semon_test
```

The thin `src/test_entry.cpp` delegates to `core::runSelfTests()`, which exercises:

1. native page allocation and page-chain telemetry;
2. direction-agnostic pin locking;
3. `Virtual3PinGate` AND constraint propagation;
4. deterministic PRNG sequence equality;
5. mathematical parsing and AST allocation;
6. model-to-circuit synthesis.

Additional fuzzing, cryptographic forward-propagation vectors, allocator stress tests, and persistence round-trip tests should be added here as separate harness translation units or data files.
