# Quantum Circuit Simulation

## Version 0.3.5

A C++17 CPU statevector simulator and quantum machine-learning research framework. Version 0.3.5 adds a portable Windows launcher, separate Debug and Release library exports, and standalone library consumers in `lib-examples/`.

The engine is organized as reusable libraries. Console output is intentionally kept in `engine/console` as a temporary demonstration layer so a future Qt frontend can replace it without changing the simulation API.

## Current capabilities

- Statevector simulation with X, Y, Z, H, T, RX, RY, RZ, CX, CY, CZ, SWAP, CCX, and multi-controlled X
- Quantum/classical registers, parameter binding, arbitrary matrix operators, controlled operators, barriers, measurement collapse, probabilities, and circuit composition
- QFT, inverse QFT, Walsh-Hadamard, phase estimation, amplitude amplification, teleportation, superdense coding, Deutsch, Deutsch-Jozsa, Bernstein-Vazirani, and Simon scaffolding
- Grover abstractions: `GroverOracle`, `GroverOperator`, and `GroverSearch`
- Shor decomposition: GCD, modular multiplication, modular power, modular exponentiation, period-finding circuit, inverse QFT, and classical post-processing
- VQE and QAOA with Pauli Hamiltonians, expectation values, graph cost functions, and coordinate-descent optimization
- QNN feature maps, parameterized ansatz circuits, expectation/probability outputs, multiple observables/output qubits, batch inference, MSE/BCE losses, training, forward passes, backward passes, and parameter-shift gradients

## Layout

```text
engine/
├── include/{core,registers,algorithms,variational,qnn}
├── src/{core,registers,algorithms,variational,qnn}
├── console/                 temporary console presentation helper
├── CMakeLists.txt
└── main.cpp
examples/                    independent programs with one main() each
lib-examples/                independent consumers using #include <QCS>
docs/manual.md               detailed API and backend manual
docs/performance.md         complexity and memory reference
```

## Quick QNN example

```cpp
QNN::QNN model;
model.setFeatureMap([](const std::vector<double>& x) {
    QuantumCircuit circuit(1);
    circuit.ry(0, x.at(0));
    return circuit;
});
model.setAnsatz([](const std::vector<double>& theta) {
    QuantumCircuit circuit(1);
    circuit.ry(0, theta.at(0));
    return circuit;
});
model.setParameters({0.35});
model.setOutputMode(QNN::OutputMode::Probability);
model.setOutputQubit(0);
double prediction = model.forward({3.141592653589793});
```

For expectation output, configure one or more `Variational::Hamiltonian` observables. `forwardValues()` returns a vector for multi-output models; `forward()` remains a scalar convenience call.

## Build on Windows

Requirements: Windows, Visual Studio 2019/2022 or Build Tools with the **Desktop development with C++** workload, and PowerShell. CMake 3.20+ is supported; `engine/run.bat` uses an installed CMake when available and otherwise downloads a pinned portable CMake copy into `engine/tools/`. Internet access is only required for that first download. The compiler is not downloaded by the project.

```text
engine\run.bat
```

Choose Debug or Release. The script configures, builds, installs the selected QCS library package, and runs `QuantumSimulationStudio`. Every `.cpp` file in `examples/` becomes an individual executable. Versioned launcher build folders prevent stale caches from older project copies; the installed packages are written to `engine/build/qcs-install/debug` and `engine/build/qcs-install/release`.

If the preflight reports an incompatible system, install the C++ workload named above and run again. If CMake cannot be downloaded, install CMake 3.20+ and add it to `PATH`, or provide internet access for the local bootstrap.

## Build and use as an installed library

The engine exports a CMake package named `QCS` and an imported target named `QCS::QCS`. Install it to a prefix, then configure the consumer examples:

```powershell
cmake -S engine -B engine/build/release -DCMAKE_INSTALL_PREFIX="$PWD/engine/build/qcs-install/release"
cmake --build engine/build/release --config Release --target quantum_engine
cmake --install engine/build/release --config Release
cmake -S lib-examples -B engine/build/lib-examples -DCMAKE_PREFIX_PATH="$PWD/engine/build/qcs-install/release"
cmake --build engine/build/lib-examples --config Release
```

For Debug consumers, use `engine/build/debug` and `engine/build/qcs-install/debug` in the equivalent commands.

Consumer code includes the public umbrella header and links the exported target:

```cpp
#include <QCS>

int main()
{
    QuantumCircuit circuit(2);
    circuit.h(0);
    circuit.cx(1, 0);
    const SimulationResult result = QuantumSimulator().run(circuit);
}
```

See `lib-examples/CMakeLists.txt` and `lib-examples/README.md` for the standalone consumer setup and complete algorithm examples.

## Validation examples

- `BellState.cpp` and `GHZState.cpp`: known entangled-state amplitudes
- `GroverSearch.cpp` and `GroverHandBuilt.cpp`: reusable and explicit Grover circuits
- `ShorFactoring.cpp` and `ShorFactoringHandBuilt.cpp`: reusable and explicit Shor demonstrations
- `VQE.cpp` and `QAOA.cpp`: variational optimization
- `QNNClassification.cpp` and `QNNRegression.cpp`: predictive QNN examples

Qubit 0 is the least-significant bit of a basis index. Bell has probability 0.5 at indices 0 and 3; GHZ has probability 0.5 at indices 0 and 7.

## Scope

The project is a transparent CPU reference implementation, not a production hardware simulator. Practical memory is usually reached well before the configured 32-qubit ceiling. Noise, shot-count APIs, scalable reversible arithmetic, advanced optimizers, automatic differentiation for arbitrary operators, GPU/distributed backends, serialization, and Qt integration are future work.

See [docs/manual.md](docs/manual.md) and [docs/performance.md](docs/performance.md).
