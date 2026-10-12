# Quantum Circuit Studio — User Manual

Quantum Circuit Studio is a C++17 quantum-circuit simulation engine with
variational optimization and a small quantum machine-learning layer. This manual
introduces the current capabilities, shows how to assemble circuits from the
engine API, and gives working patterns for elementary circuits, Grover search,
phase estimation and Shor-style factoring, VQE, QAOA, and QNNs.

The source examples are useful references, but they are not application build
targets. The snippets below are intended to be pasted into a `main.cpp` (or
another C++ source compiled and linked to the engine) and adapted to the problem.

## 1. What the project provides

### Engine capabilities

- Ordered circuit layouts made from standalone qubits and fixed-width quantum
  registers.
- A separate operation timeline containing common one-, two-, and three-qubit
  gates, parameterized rotations, custom unitary matrices, barriers, measurement,
  and sampling.
- Dense state-vector simulation with register-level QFT/IQFT, Walsh-Hadamard,
  Grover diffusion, and custom register-unitary operations.
- Explicit validation of operation metadata, wire/register references, matrix
  dimensions, finite matrix values, and unitarity before each simulation run.
- Classical helpers for modular arithmetic, continued fractions, register
  probability marginalization, and QPE timeline unrolling.
- Parameterized ansatz circuits, Hamiltonian expectations, and Adam-based
  VQE/custom-objective optimization.
- QNN forward evaluation, observable and probability readout, losses, gradients,
  batch evaluation, and a full-batch Adam training helper.

### Current limitations

- The simulator stores a dense state vector of \(2^n\) complex amplitudes for
  \(n\) wires. Memory and execution cost therefore grow exponentially; it is
  intended for small circuits and teaching/prototyping, not large-scale
  simulation.
- It is an ideal state-vector simulator: there is no noise model, density-matrix
  simulation, hardware backend, GPU backend, or distributed execution.
- Measurement and sampling print outcomes but do not collapse or update the
  state used by later operations. There is no classical feed-forward or
  persistent classical-register result API at present.
- The examples demonstrate circuit construction and API usage. They do not
  establish algorithmic advantage, production readiness, real-world model
  accuracy, or performance parity with another framework.
- The variational optimizers use repeated simulator evaluations, Adam, and
  centered finite-difference gradients. They do not guarantee a global optimum.
- QNN parameter-shift gradients are appropriate for the standard single-use
  rotation parameters in the documented examples. Shared parameters and
  parameters that scale gate angles can require a different gradient treatment.
- Algorithm examples under `example/` are source references and are not
  compiled into the application by CMake.

## 2. Build and run

Use the project-provided Windows script from the repository root:

```bat
run.bat debug
```

The script configures/builds the application and runs it. It also performs the
project's CMake configure step; this manual does not require changing
`CMakeLists.txt` to build the engine. To try an algorithm snippet, replace the
body of `main.cpp` temporarily, run the script, and restore or save your desired
entry point afterward. The standalone sources in `example/` are reference
programs and are deliberately not application targets.

The engine is C++17. Public headers are under `engine/include`; CMake exports
the core, register, utility, variational, and QML include directories to the
application target.

## 3. The common circuit workflow

Every direct circuit simulation follows the same separation of concerns:

1. **Describe the layout** with `quantum::circuit`; append registers before
   creating register views.
2. **Record operations** with `quantum::operators`. Calls append instructions;
   they do not immediately simulate gates.
3. **Run** the `quantum::simulator` with the layout and timeline.
4. **Inspect** the state vector, sampler/measurement output, and duration.

```cpp
#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <iostream>

int main()
{
    quantum::circuit circuit;
    circuit.append(registers::q_register(2));
    const auto pair = circuit(0);

    quantum::operators operations;
    operations.h(pair[0]);
    operations.cx(pair[1], pair[0]);
    operations.sampler(pair, 1024);

    quantum::simulator simulator(circuit, operations);
    simulator.run();
    std::cout << "Gate-loop duration: " << simulator.duration().count() << " ms\n";
}
```

The first argument of `cx` is its target and the second is its control. Wire
index zero is the least-significant bit in a state-vector basis index. Register
IDs count register blocks, while global wire indices also include standalone
qubits. Prefer circuit views such as `pair[0]` when possible: view-based
operations retain their source-circuit identity and can be validated against
the simulator's circuit.

For sampler histograms of correlated qubits, sample the whole register
(`sampler(pair, shots)`). Sampling each wire separately gives only separate
marginals, not the joint distribution.

### Drawing a circuit canvas

The Qt circuit canvas displays an existing circuit layout and operation
timeline. Create a `QApplication`, build the circuit and operations, then pass
both to `app::circuit_canvas`, call `draw()` and `show()`, and enter the Qt event
loop:

```cpp
#include "circuit.hpp"
#include "circuit_canvas.hpp"
#include "operators.hpp"
#include "q_register.hpp"

#include <QApplication>
#include <QString>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    quantum::circuit circuit;
    circuit.append(registers::q_register(2));

    quantum::operators operations;
    operations.h(circuit(0)[0]);
    operations.cx(circuit(0)[1], circuit(0)[0]);

    app::circuit_canvas canvas(
        circuit, operations, QStringLiteral("Bell State"));
    canvas.draw();
    canvas.resize(1000, 480);
    canvas.show();
    return application.exec();
}
```

The canvas keeps references to the circuit and operations, so both must remain
alive while the window is open. See the complete
[canvas example](../example/canvas/elementary/BellState.cpp).

## 4. Build a Bell state

A Bell pair is the smallest example of entanglement. A Hadamard creates a
superposition on the first wire, then CX correlates the second wire with it:

```cpp
quantum::circuit circuit;
circuit.append(registers::q_register(2));
const auto bell = circuit(0);

quantum::operators operations;
operations.h(bell[0]);
operations.cx(bell[1], bell[0]);
operations.sampler(bell, 1024);

quantum::simulator simulator(circuit, operations);
simulator.run();
std::cout << "Bell-state gate-loop duration: "
          << simulator.duration().count() << " ms\n";
```

The ideal register histogram has outcomes `00` and `11`; finite-shot counts
fluctuate. This project samples the joint distribution without collapsing the
state vector. See [the Bell-state example](../example/elementary/BellState.cpp)
for a complete source file.

## 5. Grover search

Grover search alternates a phase oracle with amplitude amplification. The engine
provides the diffusion reflection, but the marked-state oracle is built by the
caller from ordinary gates. The following pattern marks `|101>` on a three-wire
register:

```cpp
quantum::circuit circuit;
circuit.append(registers::q_register(3));
const auto search = circuit(0);

quantum::operators operations;
operations.walshHadamard(search);

for (int iteration = 0; iteration < 2; ++iteration) {
    operations.x(search[1]);          // Convert the zero bit to a one-control
    operations.h(search[2]);
    operations.ccx(search[2], search[0], search[1]);
    operations.h(search[2]);          // Phase kickback marks |101>
    operations.x(search[1]);          // Restore the original basis encoding
    operations.amplitudeAmplification(search, 1);
}
operations.sampler(search, 1024);

quantum::simulator simulator(circuit, operations);
simulator.run();
std::cout << "Grover gate-loop duration: "
          << simulator.duration().count() << " ms\n";
```

The iteration count depends on the search-space size and number of marked
states; two iterations are specifically used in the three-wire reference
example. Too many iterations can reduce the marked-state probability. This
example assumes a known phase oracle and does not synthesize an oracle from an
arbitrary predicate. See [GroverSearch.cpp](../example/grover/GroverSearch.cpp).

## 6. QFT and phase estimation

### QFT

Register-level transforms are direct timeline operations:

```cpp
quantum::circuit circuit;
circuit.append(registers::q_register(3));
const auto frequency = circuit(0);

quantum::operators operations;
operations.x(frequency[0]);
operations.x(frequency[1]);
operations.qft(frequency);

quantum::simulator simulator(circuit, operations);
simulator.run();
simulator.printStateVector();
```

Use `iqft(registerView)` for the inverse. See the [QFT](../example/shor/QFT.cpp)
and [IQFT](../example/shor/IQFT.cpp) examples.

### Quantum phase estimation

`quantum::utils::unrollQuantumPhaseEstimation` appends Hadamards on the phase
register, invokes a user callback for each controlled power \(U^{2^k}\), and
appends the inverse QFT. It does not define or validate the mathematical oracle
for the user. The target register must be prepared in an eigenstate of the
unitary represented by the callback.

```cpp
#include "quantum_helpers.hpp"

quantum::circuit circuit;
circuit.append(registers::q_register(3)); // phase register
circuit.append(registers::q_register(1)); // eigenstate/work register
const auto phase = circuit(0);
const auto target = circuit(1);

quantum::operators operations;
operations.x(target[0]);
operations.h(target[0]); // Prepare |->, an eigenstate of X with eigenvalue -1

quantum::utils::unrollQuantumPhaseEstimation(
    operations, phase, target,
    [](quantum::operators& timeline,
       const quantum::qubit_view& control,
       const quantum::register_view& targetRegister,
       std::size_t power) {
        if ((power & 1U) != 0) {
            timeline.cx(targetRegister[0], control);
        }
    });
operations.sampler(phase, 256);

quantum::simulator simulator(circuit, operations);
simulator.run();
```

The helper expects the callback to implement the controlled powers consistently
with its `power` argument. See
[PhaseEstimation.cpp](../example/shor/PhaseEstimation.cpp) for a complete
reference.

## 7. Shor-style order finding and factoring

The repository's Shor example is a small exact-statevector demonstration for
factoring 15 with base 2. It combines quantum phase estimation with classical
post-processing:

1. Prepare phase and work registers; initialize the work register to \(|1\rangle\).
2. Unroll QPE, using modular multiplication by \(a^{2^k}\bmod N\) as each
   controlled unitary.
3. Run the simulator.
4. Marginalize the final state vector onto the phase register.
5. Sample candidate phases and inspect continued-fraction convergents for an
   order \(r\) satisfying \(a^r \equiv 1 \pmod N\).
6. If \(r\) is even, attempt the classical GCD factor extraction.

The central callback follows this pattern:

```cpp
quantum::utils::unrollQuantumPhaseEstimation(
    operations, phaseRegister, workRegister,
    [](quantum::operators& timeline,
       const quantum::qubit_view& control,
       const quantum::register_view& target,
       std::size_t power) {
        const std::uint64_t multiplier =
            quantum::utils::modularPower(base, power, modulus);
        const auto matrix = quantum::utils::modularMultiplicationMatrix(
            target.size(), multiplier, modulus);
        timeline.applyControlledRegister(target, control, matrix);
    });
```

Post-processing can use the final state vector instead of a printed sampler
histogram:

```cpp
const auto probabilities =
    quantum::utils::calculateRegisterProbabilities(simulator.getStateVector(), phaseRegister);
const std::uint64_t order =
    quantum::utils::deducePeriodFromPhaseDistribution(probabilities, base, modulus);
```

These utilities and dense register matrices are intended for small demonstrations.
The helper samples candidate outcomes from the exact marginal distribution
obtained from the state vector; it is not a scalable, hardware-executed Shor
implementation. The routine can fail to recover a useful order, and the
classical factor step can fail to produce nontrivial factors. See
[ShorFactoring.cpp](../example/shor/ShorFactoring.cpp) for checks and complete
error-path handling.

## 8. Variational algorithms: VQE and QAOA

### VQE with a Hamiltonian

An ansatz maps a parameter vector to a circuit. A separate
`quantum::costfxn::Hamiltonian` defines the scalar energy objective:

```cpp
#include "ansatz.hpp"
#include "costfxn.hpp"
#include "optimizer.hpp"

quantum::variational::ansatz ansatz(2, 2);
quantum::costfxn::Hamiltonian hamiltonian;
hamiltonian.append(-1.0, "II");
hamiltonian.append(0.5, "ZI");
hamiltonian.append(0.5, "IZ");
hamiltonian.append(0.2, "XX");

quantum::variational::eigensolver(300, 1e-7, ansatz, hamiltonian);
std::cout << "Best energy: " << ansatz.getOptimalEnergy() << '\n';
```

The default hardware-efficient ansatz uses Ry/Rz rotations and nearest-neighbor
CX entanglers. For custom topologies, supply an explicit parameter count and a
builder callback. `eigensolver` stores its results on the ansatz. See
[vqe.cpp](../example/variational/vqe.cpp).

### QAOA with a caller-defined cost

QAOA circuit topology and problem cost are both user code. For the two-vertex
Max-Cut example, the ansatz prepares \(|+\rangle^{\otimes2}\), applies one
cost/mixer layer, and the cost callback sums the probabilities of the cut
states. Since `approximator` minimizes, the callback returns negative cut
value:

```cpp
using StateVector = quantum::costfxn::StateVector;
quantum::variational::ansatz qaoa(
    2, 2,
    [](quantum::operators& ops, const std::vector<double>& theta) {
        ops.h(0);
        ops.h(1);
        ops.cx(1, 0);
        ops.rz(1, theta[0]);
        ops.cx(1, 0);
        ops.rx(0, 2.0 * theta[1]);
        ops.rx(1, 2.0 * theta[1]);
    });

const auto negativeExpectedCut = [](const StateVector& state) {
    double cut = 0.0;
    for (std::size_t basis = 0; basis < state.size(); ++basis) {
        if (((basis & 1U) != 0) != ((basis & 2U) != 0)) {
            cut += std::norm(state[basis]);
        }
    }
    return -cut;
};
quantum::variational::approximator(300, qaoa, negativeExpectedCut, 1e-7);
std::cout << "Best expected cut: " << -qaoa.getOptimalEnergy() << '\n';
```

The built-in optimizer uses finite differences and a fixed learning rate; it is
not a general configurable optimizer framework. See
[qaoa.cpp](../example/variational/qaoa.cpp).

## 9. Quantum neural networks

`quantum::ml::qnn` combines a feature-map callback, a variational ansatz, and a
readout. It is a manager for a two-stage circuit pipeline, not a subclass of
`circuit` or `ansatz`. Training returns the optimized weight vector and retains
the trained weights and final loss on the model for inference and persistence.

This classification template maps three student features to three qubits,
reads the probability of wire zero, and trains against placed/not-placed labels:

```cpp
#include "qnn.hpp"

const std::size_t numQubits = 3;
quantum::variational::ansatz ansatz(
    numQubits, 4,
    [](quantum::operators& ops, const std::vector<double>& theta) {
        ops.ry(0, theta.at(0));
        ops.ry(1, theta.at(1));
        ops.ry(2, theta.at(2));
        ops.cx(1, 0);
        ops.rz(0, theta.at(3));
    });

quantum::ml::qnn model(
    numQubits,
    [](quantum::operators& ops, const std::vector<double>& features) {
        ops.rx(0, features.at(0)); // normalized CGPA
        ops.rx(1, features.at(1)); // major flag
        ops.rx(2, features.at(2)); // minor flag
    },
    ansatz);
model.setOutputMode(quantum::ml::OutputMode::Probability);
model.setOutputQubit(0);

const std::vector<std::vector<double>> inputs = {
    {0.92, 1.0, 0.0},
    {0.65, 0.0, 1.0},
    {0.85, 1.0, 1.0},
    {0.58, 0.0, 0.0}
};
const std::vector<double> targets = {1.0, 0.0, 1.0, 0.0};
const std::vector<double> initialWeights = {0.1, -0.2, 0.5, 0.15};
const double before = model.loss(inputs, initialWeights, targets);
const std::vector<double> weights =
    model.train(inputs, targets, initialWeights, 100, 0.08);
const std::vector<double> placementProbabilities =
    model.forwardBatch(inputs, weights);
const double after = model.loss(inputs, weights, targets);
```

Automatic loss selection uses binary cross-entropy for probability output and
mean-squared error for expectation output. Replace the illustrative rows with
representative training examples and evaluate on held-out data for a meaningful
application. The small template is not evidence of predictive performance.
See [QNN_Placement_Predictor.cpp](../example/qml/QNN_Placement_Predictor.cpp),
[QNN_Classification.cpp](../example/qml/QNN_Classification.cpp), and
[QNN_Regression.cpp](../example/qml/QNN_Regression.cpp).

## 10. Timing and framework comparison

Each example can be timed, but define the measured boundary before comparing
numbers. `simulator.duration()` sums per-operation execution intervals and
excludes timeline validation, state initialization, and gate-trace printing.
Measurement/sampling output occurs inside those operation intervals unless
disabled. This duration is useful for inspecting the engine's execution loop,
but it is **not** directly comparable to a framework's end-to-end backend call.

For a fair simulator-versus-Qiskit comparison, prefer an externally measured
wall-clock interval around equivalent work on both sides. Use the same circuit,
wire count, precision, simulator method, shot count, and inclusion/exclusion
rules. Decide whether compilation/transpilation, state initialization, result
sampling, classical post-processing, optimizer iterations, and output printing
are included; document those choices beside the averages. Repeat each run and
report the mean over the same number of repetitions. This is especially
important for VQE, QAOA, and QNN, where training includes many circuit
evaluations and classical updates.

The table records the average wall-clock times from the runs shown. In this
specific setup, QCS had the lower average in eight of the nine workloads;
Qiskit had the lower average for QNN classification. This is an observation
about these implementations, workload sizes, machine, and timing boundary—not
a general performance claim. The speedup column is
`Qiskit average / this simulator average`; values above `1x` indicate a lower
average time for this simulator in this table.

| Workload and shared settings | Runs | This simulator average (ms) | Qiskit average (ms) | Speedup |
|---|---:|---:|---:|---:|
| Bell state: 2 wires, 1024 shots | 8 | 0.5417 | 79.4593 | <span style="color:green">146.69x</span> |
| Grover search: 3 wires, 2 iterations, 1024 shots, marked state `101` | 8 | 0.5189 | 75.4254 | <span style="color:green">145.36x</span> |
| QFT: 3 wires | 8 | 0.1634 | 0.8092 | <span style="color:green">4.95x</span> |
| Phase estimation: 3 phase wires + 1 eigenstate wire, 256 shots | 8 | 0.3005 | 82.2777 | <span style="color:green">273.80x</span> |
| Shor order-finding: 4 phase + 4 work wires, \(N=15\), \(a=2\), 1024 shots | 8 | 1.9599 | 177.5807 | <span style="color:green">90.61x</span> |
| VQE: 2 wires, 5 Hamiltonian terms, maximum 300 optimizer iterations | 8 | 216.9733 | 1222.5590 | <span style="color:green">5.63x</span> |
| QAOA: 2 wires, 1-edge graph, depth 1, maximum 300 optimizer iterations | 8 | 39.0784 | 206.5481 | <span style="color:green">5.29x</span> |
| QNN classification: 3 wires, 8 training + 4 test rows, 11 features, 1 epoch, batch size 4 | 8 | 409.2483 | 174.2277 | <span style="color:red">0.43x</span> |
| QNN regression: 1 wire, 3 rows, 60 epochs, batch size 3 | 8 | 8.2502 | 123.0271 | <span style="color:green">14.91x</span> |

The C++ and Qiskit examples use matching workload sizes and optimization
settings where applicable. These are end-to-end wall-clock measurements:
Qiskit shot-based runs include backend transpilation, and QNN runs include
preparation, training, prediction, and metrics. They exclude process startup
and console output. Results depend on the machine, compiler/build mode,
framework versions, simulator settings, and timing boundary; do not interpret
these measurements as a general performance guarantee.

## 11. Architecture references

- [Circuit layouts and views](engine/circuit.md)
- [Gate timelines and operators](engine/operators.md)
- [Quantum registers](engine/registers.md)
- [Variational circuits and optimization](engine/variational.md)
- [QNN modules and gradient behavior](engine/qnn.md)
- [Simulator execution, validation, and measurement](engine/simulator.md)
