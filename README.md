# Quantum Circuit Library

A lightweight C++17 quantum circuit simulation project focused on statevector simulation and bit-manipulation-based quantum gate application.

> **Status:** v0.1 — foundational CPU implementation / research prototype.

## Overview

The project represents a quantum circuit as an ordered collection of gates and executes those gates on a statevector.

The current architecture is intentionally simple:

```text
QuantumCircuit
      │
      │ vector<Gate>
      ▼
QuantumSimulator
      │
      │ gate application
      ▼
Statevector
```

The simulator does **not** construct full \(2^n \times 2^n\) gate matrices. Instead, it works directly on the statevector using basis-state indexing and bit manipulation.

## Current Features

- C++17 implementation
- Statevector simulation
- Ordered gate storage using `std::vector<Gate>`
- Bit-manipulation-based gate application
- Single-qubit gates:
  - X
  - Y
  - Z
  - H
  - T
  - RX
  - RY
  - RZ
- Two-qubit gates:
  - CX / CNOT
  - CY
  - CZ
  - SWAP
- Three-qubit gate:
  - CCX / Toffoli
- Basic execution-time measurement
- CMake build configuration
- Windows batch build/run helper

## Example

```cpp
QuantumCircuit circuit(2);

circuit.H(0);
circuit.CX(1, 0);

QuantumSimulator simulator;

// Run the circuit on a 2-qubit statevector.
simulator.run(circuit, state, 1 << 2);
```

The Bell-state circuit above should produce amplitudes corresponding approximately to

```text
|00> : 1/sqrt(2)
|11> : 1/sqrt(2)
```

with the other basis-state amplitudes approximately zero.

## Building

### Requirements

- C++17-compatible compiler
- CMake 3.20 or newer
- A standard C++ build environment

### CMake

From the project root:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

The exact executable location depends on the generator/compiler configuration.

### Windows

The repository also contains `run.bat` for a convenient local build/run workflow.

## Project Structure

```text
Quantum-Circuit-Simulation/
│
├── engine/
│   ├── core/
│   │   ├── QuantumCircuit.cpp
│   │   └── QuantumSimulator.cpp
│   │
│   └── headers/
│   │   ├── QuantumCircuit.h
│   │   └── QuantumSimulator.h
│   │
│   ├── main.cpp
│   ├── CMakeLists.txt
│   └── run.bat
│
├── docs/
│   └── manual.md
│
├── README.md
├── LICENSE
└── .gitignore
```

## Design Philosophy

The v0.1 implementation deliberately prioritizes clarity and correctness over aggressive optimization.

The long-term plan is to keep the public simulation interface stable while replacing or adding optimized execution backends.

## Limitations of v0.1

This version is a foundational engine rather than a complete quantum-computing framework.

Currently it does not aim to provide:

- Measurement / shot sampling
- Noise models
- Density-matrix simulation
- GPU execution
- Distributed simulation
- Advanced circuit optimization / transpilation
- Hardware execution
- Full production-grade error handling

These are planned areas for future versions.

## Documentation

See [`docs/manual.md`](docs/manual.md) for the technical description of the statevector representation, gate storage, bit manipulation, and simulation architecture.

## License

This project is released under the MIT License. See [`LICENSE`](LICENSE).
