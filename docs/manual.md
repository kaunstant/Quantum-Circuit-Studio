# Quantum Circuit Simulation Manual

## 1. Release scope

Version 0.2 is the consolidated foundation release. It contains the original 1. statevector engine, the 2. programming core, the 3. algorithm library, the 4. Grover/Shor modules, and the 5. variational modules. The milestone names remain in this manual because they explain the architecture, but the project version is v0.2.

## 2. Architecture and layout

```text
QuantumCircuit -> operation records -> QuantumSimulator -> statevector
                                      |-> classical results
                                      |-> expectation evaluators
```

```text
engine/include/core          circuit and simulator interfaces
engine/include/registers     quantum/classical register interfaces
engine/include/algorithms    reusable algorithm builders
engine/include/variational   Hamiltonian, VQE, QAOA, optimizer interfaces
engine/src/core              statevector implementation
engine/src/registers         register implementation
engine/src/algorithms        algorithm implementation
engine/src/variational       variational implementation
engine/console               temporary console formatting helpers
examples                     independent demonstration programs
```

The `quantum_engine` static library contains simulation and algorithm logic. Console output is kept in a separate temporary area so a future Qt frontend can replace it without coupling UI code to the engine internals.

## 3. Statevector convention

For `n` qubits, the statevector contains `2^n` complex amplitudes and starts in index 0. Qubit 0 is the least-significant bit. For three qubits, index 5 represents `|101>`.

Built-in gates use direct statevector kernels in `O(2^n)` time and approximately `16 * 2^n` bytes for double-precision complex amplitudes. Full system matrices are avoided except when a caller explicitly supplies a small matrix operator.

## 4. Existing gate and register API

Both uppercase v0.1 calls and lowercase v0.2 calls remain available:

```cpp
Register::QuantumRegister q(4);
Register::ClassicalRegister c(4);
QuantumCircuit circuit(4);

circuit.h(q);
circuit.measure(q, c);
```

Parameterized rotations use named handles:

```cpp
Parameter theta("theta");
QuantumCircuit ansatz(1);
ansatz.ry(0, theta);
ansatz.bind(theta, 0.732);
```

Unbound parameters cause simulation to throw. Matrix operations use a `2^k` by `2^k` matrix for `k` target qubits. Circuits with equal widths can be combined using `append` or `operator+`.

## 5. Circuit-library algorithms

The reusable builders include QFT, inverse QFT, Walsh-Hadamard, phase estimation, amplitude amplification, teleportation, superdense coding, Deutsch, Deutsch-Jozsa, Bernstein-Vazirani, and Simon scaffolding:

```cpp
QuantumCircuit qft = Algorithms::qft(0, 4);
QuantumCircuit inverse = Algorithms::inverseQft(0, 4);
QuantumCircuit bv = Algorithms::bernsteinVazirani({1, 0, 1});
```

The standalone examples under `examples/` provide one `main()` per algorithm and print statevector tables through `engine/console/StatePrinter.hpp`.

## 6. Bell and GHZ checks

```cpp
QuantumCircuit bell(2);
bell.h(0);
bell.cx(1, 0);

QuantumCircuit ghz(3);
ghz.h(0);
ghz.cx(1, 0);
ghz.cx(2, 1);
```

The Bell state has probability 0.5 at basis indices 0 and 3. The GHZ state has probability 0.5 at indices 0 and 7. `examples/BellState.cpp` and `examples/GHZState.cpp` print these tables.

## 7. Grover

```cpp
Algorithms::GroverOracle oracle(3, 5);
Algorithms::GroverSearch search(oracle);
SimulationResult result = search.run();
```

`GroverOracle` marks a basis state, `GroverOperator` combines the oracle and diffusion operator, and `GroverSearch` selects the standard iteration count. `examples/GroverSearch.cpp` uses the reusable API. `examples/GroverHandBuilt.cpp` writes the oracle and diffusion steps directly.

## 8. Shor

```cpp
Algorithms::Shor shor(15);
Algorithms::ShorFactors factors = shor.factor();
QuantumCircuit periodFinding = shor.periodFindingCircuit(4);
```

The reusable implementation is divided into greatest common divisor, modular multiplication, modular power, classical order finding, modular exponentiation, inverse QFT, and classical factor post-processing. `examples/ShorFactoring.cpp` calls these routines. `examples/ShorFactoringHandBuilt.cpp` spells out the small demonstration for 15 in one file.

The current arithmetic uses small reversible permutation matrices and deterministic classical fallback, so it is appropriate for teaching and validation rather than large integer factoring.

## 9. Variational quantum computing

The consolidated v0.2 release includes this pipeline:

```text
ParameterizedCircuit -> Parameters -> Expectation values -> Cost function -> Optimizer
```

VQE minimizes:

$$E(\theta) = \langle \psi(\theta) | H | \psi(\theta) \rangle$$

```cpp
Variational::Hamiltonian hamiltonian({{1.0, "Z"}});
Variational::ParameterizedCircuit ansatz =
    [](const std::vector<double>& parameters)
    {
        QuantumCircuit circuit(1);
        circuit.ry(0, parameters.at(0));
        return circuit;
    };

Variational::VQE vqe(ansatz, hamiltonian);
auto result = vqe.minimize({0.4});
```

QAOA alternates graph cost phases and RX mixer layers. Parameters are ordered as `gamma_0, beta_0, gamma_1, beta_1, ...`, and the exposed cost is negative expected cut value so minimization maximizes the cut.

`examples/VQE.cpp` reaches the ground energy `-1` for `H = Z`. `examples/QAOA.cpp` reaches negative cut cost `-1` for a one-edge graph.

## 10. Validation and limits

Build all examples with:

```bash
cmake -S engine -B engine/build/debug
cmake --build engine/build/debug --config Debug
```

This release is a CPU statevector reference implementation. It does not yet provide scalable reversible arithmetic, noise, density matrices, shot-count APIs, gradients, QNN inference, serialization, circuit optimization, GPU/distributed backends, hardware execution, or Qt integration.