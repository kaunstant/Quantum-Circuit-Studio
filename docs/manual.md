# Quantum Circuit Simulation Manual

## 1. Release identity

Version 0.3.5 is the portable packaging release. It contains the consolidated 0.3 research implementation plus a Windows launcher that bootstraps CMake locally, separate Debug/Release QCS exports, and standalone library examples.

## 2. Design goals

The project favors readable reference implementations, explicit statevector behavior, stable public contracts, and modular boundaries suitable for future Qt, GPU, MPS, and hardware backends.

## 3. Project structure

```text
engine/include/core          QuantumCircuit and QuantumSimulator
engine/include/registers     QuantumRegister and ClassicalRegister
engine/include/algorithms    QFT, Grover, Shor, and algorithm builders
engine/include/variational   Hamiltonian, VQE, QAOA, optimizer
engine/include/qnn           QNN, gradients, losses
engine/src/core              core implementations
engine/src/registers         register implementations
engine/src/algorithms        algorithm implementations
engine/src/variational       variational implementations
engine/src/qnn               QNN implementations
engine/console               temporary console table output
examples                     one independent main() per demonstration
docs                         manual and performance reference
```

## 4. Core architecture

`QuantumCircuit` records ordered operations. `QuantumSimulator` executes them on a statevector. Variational and QNN modules build circuits through public APIs rather than duplicating simulator kernels.

## 5. Statevector representation

For `n` qubits, the state contains `2^n` complex amplitudes. The initial state is `|00...0>`, stored at index 0. Amplitude probability is `norm(amplitude)`.

## 6. Qubit indexing

Qubit 0 is the least-significant bit. For three qubits, index 5 represents `|101>`. This convention is shared by gates, matrix operators, measurement, QNN probabilities, and console output.

## 7. Built-in gates

The engine supports X, Y, Z, H, T, RX, RY, RZ, CX, CY, CZ, SWAP, CCX, and multi-controlled X. Uppercase legacy names and lowercase names are both supported.

## 8. Registers

`QuantumRegister` provides named groups of `Qubit` values. `ClassicalRegister` provides named classical storage metadata. Register-wide operations and register measurement are available when the register widths match.

## 9. Parameters

Rotation gates can store a named `Parameter` until a numerical value is supplied with `bind`. Simulation rejects unbound parameters. This is the foundation for VQE, QAOA, and QNN training.

## 10. Arbitrary operators

`apply(matrix, qubits)` accepts a dense `2^k x 2^k` operator for `k` targets. `controlled(matrix, control, targets)` applies it only when the control is one. Target order defines matrix bit order.

## 11. Circuit composition

Circuits with equal qubit counts can be appended or combined with `operator+`. Barriers are retained as compiler/scheduling markers and do not alter a statevector.

## 12. Measurement implementation

Measurement is implemented directly in the backend. The simulator computes every basis probability from the squared magnitude of its amplitude, samples a basis index, reads the requested bit, zeros amplitudes inconsistent with that result, and renormalizes the surviving amplitudes. Classical results are stored in `SimulationResult::classicalBits`. The current API performs one deterministic-seed simulation run; shot batches are future work.

## 13. Probability calculation

`QuantumSimulator::probabilities` returns one probability for each basis index. The values sum to one for a normalized state, subject to floating-point tolerance.

## 14. Bell validation

```cpp
QuantumCircuit bell(2);
bell.h(0);
bell.cx(1, 0);
```

The result has probability 0.5 at indices 0 and 3. Run `examples/BellState.cpp` to print the table.

## 15. GHZ validation

```cpp
QuantumCircuit ghz(3);
ghz.h(0);
ghz.cx(1, 0);
ghz.cx(2, 1);
```

The result has probability 0.5 at indices 0 and 7. Run `examples/GHZState.cpp` to print the table.

## 16. Circuit transformations

`Algorithms::qft`, `inverseQft`, and `walshHadamard` build reusable transformation circuits. QFT uses Hadamards, controlled phases, and final swaps; inverse QFT reverses phase signs and order.

## 17. Algorithm library

The library includes phase estimation, amplitude amplification, teleportation, superdense coding, Deutsch, Deutsch-Jozsa, Bernstein-Vazirani, and Simon scaffolding. Each has an independent example source.

## 18. Grover search

`GroverOracle` marks a basis state, `GroverOperator` appends oracle and diffusion, and `GroverSearch` chooses the standard iteration count. The hand-built example spells out the same operations.

## 19. Shor factoring

The Shor module separates GCD, modular multiplication, modular power, classical order finding, reversible modular-exponentiation permutations, inverse QFT period finding, and classical post-processing. The small reference demonstration factors 15 as 3 times 5. It is educational and not a scalable large-integer implementation.

## 20. Variational computing

VQE evaluates and minimizes `E(theta) = <psi(theta)|H|psi(theta)>` for Pauli Hamiltonians. QAOA builds alternating graph cost phases and RX mixer layers and minimizes negative expected cut value. Coordinate descent provides a deterministic reference optimizer.

## 21. QNN model

The QNN pipeline is `FeatureMap -> Ansatz -> Statevector -> Observable or probability -> output`. `QNN::forward` provides a scalar output, `forwardValues` supports multiple observables or probability output qubits, `forwardBatch` supports batches, and `ForwardResult` exposes state and probabilities.

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

## 22. Losses, gradients, and training

Expectation outputs use squared error; probability outputs use binary cross entropy. For rotation-generated parameters, backward evaluation uses:

$$\frac{\partial f}{\partial \theta} = \frac{1}{2}\left[f\left(\theta + \frac{\pi}{2}\right) - f\left(\theta - \frac{\pi}{2}\right)\right]$$

`backward` returns one gradient per trainable parameter, `lossGradient` applies the chain rule, and `train` performs batch gradient descent. `examples/QNNClassification.cpp` predicts two classes; `examples/QNNRegression.cpp` learns cosine-like expectation values.

## 23. Console output

`engine/console/StatePrinter.hpp` prints basis labels, real amplitudes, imaginary amplitudes, and probabilities in a stable table. It is deliberately isolated so Qt code can replace it later.

## 24. Build and validation

```bash
cmake -S engine -B engine/build/debug
cmake --build engine/build/debug --config Debug
```

CMake builds the static `quantum_engine` library, the engine smoke program, and one executable for each example source. On Windows, `engine/run.bat` is the recommended entry point: it checks for MSVC, finds or downloads CMake locally, builds the selected configuration, and installs it to `engine/build/qcs-install/debug` or `engine/build/qcs-install/release`.

The supported machine must have Windows and a compatible Visual Studio or Build Tools installation with the Desktop development with C++ workload. CMake can be installed automatically by the launcher, but the compiler and Windows SDK must be supplied by the user. A missing compiler, SDK, network connection, or unsupported platform is reported during the preflight/configuration step.

## 25. Error handling

Public construction APIs validate qubit ranges, duplicate operands, matrix dimensions, register sizes, parameter state, optimizer settings, and training data. Invalid configuration throws standard exceptions such as `std::invalid_argument`, `std::out_of_range`, or `std::logic_error`.

## 26. Numerical considerations

The simulator uses double-precision complex amplitudes. Normalization and expectation values are subject to floating-point error. Examples use tolerances rather than exact equality.

## 27. Backend boundary

The current backend is a CPU statevector simulator. Circuit records and QNN builders are intentionally separate from execution so alternate backends can be introduced without rewriting algorithm code.

## 28. Current limitations

The release does not yet include noise, density matrices, shot-count APIs, scalable reversible arithmetic, advanced optimizers, automatic differentiation for arbitrary operators, serialization, GPU/distributed execution, hardware execution, or Qt integration.

## 29. Performance guidance

Statevector memory grows exponentially. The configured maximum is 32 qubits, but 32 qubits requires approximately 64 GiB for the state alone. Dense matrix operations are limited by `4^k` matrix storage. See [performance.md](performance.md) for detailed complexity tables.

## 30. Future QNN work

Natural next steps include richer observable collections, sampler-style shot outputs, trainable parameter metadata, gradient backends beyond parameter shift, configurable optimizers, validation splits, model serialization, and framework adapters.
