# Variational Circuits and Optimization

The variational module separates a parameterized circuit blueprint from its
objective function and optimizer. The ansatz defines how parameter vectors
become operation timelines. A cost function evaluates the simulated state, and
the optimizer updates parameters to reduce that cost.

## Ansatz construction

`quantum::variational::ansatz` derives from `quantum::circuit`, so the ansatz
itself supplies the wire layout used when its generated operations are
simulated. It supports a hardware-efficient default or a custom circuit
callback:

```cpp
#include "ansatz.hpp"

quantum::variational::ansatz ansatz(
    2, 4,
    [](quantum::operators& operations, const std::vector<double>& theta) {
        operations.ry(0, theta.at(0));
        operations.rz(0, theta.at(1));
        operations.ry(1, theta.at(2));
        operations.rz(1, theta.at(3));
        operations.cx(1, 0);
    });
```

The second argument declares the exact parameter count for custom circuits.
At every evaluation, `generateOperators` calls the builder with the current
vector and rejects a vector with the wrong size or non-finite values. The
builder must append gates in the same stable parameter order on every call.

The `(qubits, layers)` constructor creates the engine's default layered
hardware-efficient circuit. It uses two rotations per wire per layer (Ry then
Rz), followed by a nearest-neighbor CX chain. Its parameter count is
\(2 \times qubits \times layers\). The `(qubits, initialParams)` overload
creates that same default structure and uses the supplied vector as its
starting values.

## Hamiltonian and state-vector cost functions

Hamiltonians live in `quantum::costfxn`, not on the ansatz. A Pauli string has
one character per circuit wire; character zero acts on wire zero:

```cpp
#include "costfxn.hpp"

quantum::costfxn::Hamiltonian hamiltonian;
hamiltonian.append(-1.0, "II");
hamiltonian.append(0.5, "ZI");
hamiltonian.append(0.5, "IZ");
hamiltonian.append(0.2, "XX");
```

`Hamiltonian::expectation` computes the normalized real expectation
\(\langle\psi|H|\psi\rangle\) from a state vector. It checks that the vector
dimension matches the Hamiltonian width and rejects an empty Hamiltonian,
invalid norm, non-finite amplitudes, or a materially complex result.
`makeHamiltonianCostFunction` adapts a Hamiltonian to the generic
state-vector `CostFunction` callback.

## VQE optimization

`eigensolver(maxIterations, tolerance, ansatz, hamiltonian)` evaluates the
Hamiltonian expectation and stores the best energy, parameters, history, and
parameter trajectory on the ansatz:

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

quantum::variational::eigensolver(200, 1e-6, ansatz, hamiltonian);
const double bestEnergy = ansatz.getOptimalEnergy();
const auto& bestParameters = ansatz.getOptimalParameters();
```

The eigensolver checks that the Hamiltonian is nonempty and describes the same
number of qubits as the ansatz. It uses a fixed learning rate of `0.05`.

## QAOA and custom objectives

`approximator` takes a callback from the full state vector to a scalar cost.
This keeps graph- or problem-specific logic outside the engine's circuit and
optimizer primitives. For a Max-Cut objective, the callback can sum the
probability of basis states whose endpoint bits differ:

```cpp
using StateVector = quantum::costfxn::StateVector;
const auto negativeCut = [](const StateVector& state) {
    double cut = 0.0;
    for (std::size_t basis = 0; basis < state.size(); ++basis) {
        const bool left = (basis & 1U) != 0;
        const bool right = (basis & 2U) != 0;
        if (left != right) {
            cut += std::norm(state[basis]);
        }
    }
    return -cut;
};
quantum::variational::approximator(200, qaoa, negativeCut);
```

Because this optimizer minimizes, the example negates the expected cut value;
the reported best cut is the negative of the stored optimal cost.

## Current optimizer behavior and limits

Both optimizer entry points currently use an internal Adam update loop with
centered finite-difference gradients (step `1e-5`). This is a simulator-backed
classical optimizer, not a shot-based optimizer or a hardware execution
interface. Circuit traces are suppressed during its repeated internal
simulations.

The ansatz exposes initial parameters and the optimizer's best-result
histories, but optimization results are stored on the ansatz instance. The QNN
module has its own training and inference interface; see
[QNN architecture](qnn.md).

Dense state-vector simulation scales exponentially with qubit count.
Optimization also repeats a complete simulation for each finite-difference
evaluation, so runtime grows with both circuit size and parameter count.
The configured iteration limit and tolerance do not guarantee a global
minimum.

## Related guides

- [Circuit and operator timelines](circuit.md)
- [Hamiltonian and cost-function API](operators.md)
- [QNN training and gradients](qnn.md)
- [Simulator behavior and timing](simulator.md)

## Examples

- [VQE](../../example/variational/vqe.cpp)
- [QAOA](../../example/variational/qaoa.cpp)
