# Quantum Circuit Simulation

## Version 0.2

This release consolidates the planned 0.2 through 5 milestones into one research-oriented C++17 reference simulator. It includes the original statevector gate engine, circuit-library algorithms, Grover and Shor building blocks, and variational VQE/QAOA support.

The implementation is intentionally split into reusable modules so a future Qt frontend can depend on the engine without changing the simulation contracts.

## Features

- Statevector simulation with X, Y, Z, H, T, RX, RY, RZ, CX, CY, CZ, SWAP, and CCX
- Quantum and classical registers, parameter binding, measurements, probabilities, and circuit composition
- Arbitrary matrix operators, controlled operators, controlled phase, multi-controlled X, and barriers
- QFT, inverse QFT, Walsh-Hadamard, phase estimation, amplitude amplification, teleportation, superdense coding, Deutsch, Deutsch-Jozsa, Bernstein-Vazirani, and Simon scaffolding
- Grover abstractions: `GroverOracle`, `GroverOperator`, and `GroverSearch`
- Shor decomposition: modular arithmetic, modular exponentiation, period-finding circuit, QFT, measurement-ready state, and classical post-processing
- Variational computing: Pauli Hamiltonians, expectation values, coordinate descent, VQE, and QAOA
- Independent console examples for each algorithm

## Project layout

```text
engine/
├── include/
│   ├── core/          circuit and simulator contracts
│   ├── registers/     quantum/classical registers
│   ├── algorithms/    circuit-library, Grover, and Shor APIs
│   └── variational/   Hamiltonian, VQE, QAOA, and optimizer APIs
├── src/
│   ├── core/
│   ├── registers/
│   ├── algorithms/
│   ├── variational/
│   └── console/       temporary console presentation helpers
├── console/           temporary output helpers for examples and smoke tests
└── main.cpp
examples/              one copy-ready main program per algorithm
docs/                  technical manual
```

The console helpers are temporary by design. They live under `engine/console` so the current executable can display useful results; a future Qt layer can replace them without changing the simulation modules.

## Build

Requirements: CMake 3.20 or newer and a C++17 compiler.

```bash
cmake -S engine -B engine/build/debug
cmake --build engine/build/debug --config Debug
```

On Windows, run `engine/run.bat` and choose Debug or Release. Each `.cpp` file in `examples/` becomes an individual executable under the selected build configuration.

## Conventions

Qubit 0 is the least-significant bit of a basis index. A Bell circuit:

```cpp
QuantumCircuit bell(2);
bell.h(0);
bell.cx(1, 0);
```

has probability 0.5 at indices 0 and 3. A three-qubit GHZ circuit has probability 0.5 at indices 0 and 7.

## Development milestones

The internal plan remains visible for context: 0.1 foundation, 0.2 programming core, 0.3 circuit library, 0.4 Grover/Shor, and 0.5 variational computing. They are delivered together here as version 0.2. Future work includes QNN inference, gradients, a QNN framework, improved reversible arithmetic, noise models, alternate backends, optimization, serialization, and a Qt frontend.

See [docs/manual.md](docs/manual.md) for API details and runnable examples.