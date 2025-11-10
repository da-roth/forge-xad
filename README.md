# Forge-XAD Integration Playground

This is an experimental playground for integrating [Forge](https://github.com/da-roth/forge) (JIT compiler) with [XAD](https://github.com/da-roth/xad) (automatic differentiation library).

## Goal

Enable XAD users to:
1. Record computation tape **once**
2. JIT compile to native code using Forge
3. Execute **many times** with different inputs (10-100x speedup)

## Directory Structure

```
forge-xad/
├── CMakeLists.txt              # Main build configuration
├── extern/                     # External dependencies (submodules)
│   ├── forge/                  # Forge JIT compiler (submodule)
│   └── XAD/                    # XAD fork (submodule)
├── include/forge_xad/          # Bridge library headers
│   ├── xad_tape_converter.hpp  # XAD tape → Forge graph converter
│   └── operation_inference.hpp # OpCode inference from tape patterns
├── src/                        # Bridge library implementation
│   ├── xad_tape_converter.cpp
│   └── operation_inference.cpp
├── examples/                   # Example programs
│   ├── xad_baseline.cpp        # Standard XAD (baseline)
│   ├── xad_forge_integration.cpp  # With Forge JIT (TODO)
│   └── simple_function_test.cpp   # Basic XAD test
└── tests/                      # Unit tests (TODO)
```

## Setup

### 1. Clone with Submodules

From the TapePresso root:

```bash
git submodule update --init --recursive
```

This will fetch:
- `forge-xad/extern/forge` from https://github.com/da-roth/forge
- `forge-xad/extern/XAD` from https://github.com/da-roth/xad

### 2. Build

```bash
cd forge-xad
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 3. Run Examples

```bash
# Baseline XAD (without JIT)
./build/examples/xad_baseline_example

# Simple test
./build/examples/simple_function_test

# Forge integration (TODO - not yet implemented)
./build/examples/xad_forge_example
```

## Development Workflow

### Modifying XAD (Your Fork)

```bash
cd forge-xad/extern/XAD
git checkout -b feature/forge-backend-hooks
# Make changes to XAD source files
git add .
git commit -m "Add backend hook points"
git push origin feature/forge-backend-hooks
```

### Modifying Forge

```bash
cd forge-xad/extern/forge
git checkout -b feature/xad-integration
# Make changes to Forge source files
git add .
git commit -m "Update API for XAD integration"
git push origin feature/xad-integration
```

### Implementing the Bridge

Edit files in `forge-xad/src/` and `forge-xad/include/forge_xad/`:
- `xad_tape_converter.cpp` - Convert XAD tape to Forge graph
- `operation_inference.cpp` - Infer OpCode from tape patterns

### Updating Submodule References

After pushing changes to XAD or Forge:

```bash
cd forge-xad
git add extern/XAD extern/forge
git commit -m "Update submodule references"
# Then from TapePresso root:
cd ..
git add forge-xad
git commit -m "Update forge-xad playground"
git push
```

## Implementation Status

### ✅ Completed
- [x] Project structure
- [x] CMake build system
- [x] Submodules added (Forge + XAD)
- [x] Bridge library skeleton
- [x] Baseline XAD example

### 🚧 In Progress
- [ ] XAD tape → Forge graph converter
- [ ] Operation type inference
- [ ] Forge JIT integration example

### 📋 TODO
- [ ] Value synchronization (XAD variables ↔ Forge workspace)
- [ ] Gradient computation via compiled kernel
- [ ] Performance benchmarks
- [ ] Wrapper API (minimal user code changes)
- [ ] Unit tests
- [ ] Documentation

## Key Challenges

### 1. OpCode Inference
XAD stores operations as `(multiplier, slot)` pairs without explicit types.
We need pattern matching to infer the operation type.

Example:
```cpp
// XAD tape: [(1.0, slot_x), (1.0, slot_y)]
// Could be: Add, or linear combination
// Need to infer: OpCode::Add
```

### 2. Data-Dependent Branching
XAD allows `if` statements, but Forge requires fixed execution paths.
Detection and handling strategy needed.

### 3. Value Synchronization
XAD variables store values, Forge uses indexed workspaces.
Need bidirectional mapping: `slot → node_id → workspace_index`

## Integration Strategies (from docs)

See `docs/xadRefactor/INTEGRATION_ANALYSIS.md` for detailed analysis of:
1. **Tape-to-Graph Converter** (recommended)
2. Expression template capture
3. Replace XAD tape
4. Direct DoubleTP usage

## Performance Expectations

From Monte Carlo simulations:
- **XAD baseline**: Re-record every iteration
- **With Forge JIT**: Compile once, execute many times
- **Expected speedup**: 10-100x for >1000 iterations

## References

- Forge repo: https://github.com/da-roth/forge
- XAD fork: https://github.com/da-roth/xad
- XAD upstream: https://github.com/auto-differentiation/xad
- XAD Issue #70: https://github.com/auto-differentiation/xad/issues/70

## Contributing

This is a playground! Feel free to experiment with different approaches.

Key files to implement:
1. `src/xad_tape_converter.cpp` - Core conversion logic
2. `examples/xad_forge_integration.cpp` - Working example
3. Tests to verify correctness

## License

To be determined (depends on XAD AGPL-3.0 and Forge license compatibility)
