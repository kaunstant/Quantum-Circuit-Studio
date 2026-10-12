# Quantum simulator

The simulator executes an ordered `quantum::operators` timeline over a dense
state vector for a `quantum::circuit`. Its inputs are non-owning references:
both objects must remain alive and must not be modified concurrently for the
simulator's lifetime.

```cpp
quantum::circuit circuit;
registers::q_register reg(2);
circuit.append(reg);

quantum::operators operations;
operations.h(circuit[0]);
operations.cx(circuit[1], circuit[0]);

quantum::simulator simulator(circuit, operations);
simulator.run();
```

## State vector and wire indexing

At the start of every `run()`, the simulator validates the complete operation
timeline and initializes the state to \(|0\ldots0\rangle\). The vector contains
\(2^n\) complex amplitudes for an \(n\)-qubit circuit. Entry zero starts at one;
all other amplitudes start at zero.

Circuit wire `q[k]` is represented by bit `k` in a basis-state index. Thus the
amplitude at index `i` describes the basis state whose wire values are the bits
of `i`. The vector is shared by all gates, so gates acting on separate wires
can create and update entanglement.

State-vector memory grows exponentially with the number of circuit wires. The
simulator checks that the state dimension can be represented by `std::size_t`
and that the vector can represent that many entries. Available memory may impose
a much lower practical limit.

## Validation

Validation happens once at the beginning of `run()`, not in the constructor.
For every operation it checks:

- Every target and control index is nonnegative and less than
  `circuit.numQubits()`.
- Target and control wires within one operation are distinct.
- The operation has the expected target/control counts and recognized name.
- Rotation parameters are finite.
- Single-qubit custom matrices are finite, 2-by-2, and unitary. Custom register
  matrices are finite, square, and unitary with dimension \(2^r\), where \(r\)
  is the selected register's size.
- Register-view operations retain their originating circuit identity; the
  simulator rejects them when executed against a different circuit timeline.
- Single-wire measurements have one target and a nonnegative classical-bit
  index. Register measurements and samplers must identify every wire in the
  selected register.
- Samplers have a positive integer shot count.
- Register transforms refer to existing circuit register IDs and have
  operation-appropriate metadata.

Invalid circuit/operation combinations throw `circuit_operation_mismatch`.
Integer-index overloads can only be checked against the bound circuit's valid
wire/register ranges; overloads accepting circuit views also retain and verify
the originating circuit identity.

## Gate execution

One-wire operations update the two amplitudes that differ only at the target
bit. This avoids allocating a full matrix or a temporary copy of the state for
standard gates. Diagonal gates update only amplitudes for which their phase
condition applies. Controlled operations test the control bits before updating
the target pair.

`apply`, `controlled`, and `multi_controlled` use the concrete 2-by-2 unitary
matrix stored on the operation. Controlled matrix gates apply the same
two-amplitude update only when all control bits are one. Custom matrix
operations do not carry parameter vectors; callers that optimize such a gate
rebuild the matrix from their current parameters before appending the operation.

`applyRegister` applies a caller-supplied unitary matrix to the complete
selected register. Its matrix dimension must be \(2^r\) by \(2^r\) for a
register containing \(r\) wires. The simulator gathers each register-sized
amplitude block, multiplies it by the matrix, and writes the resulting block
back. Wires outside that register are untouched. Matrix row and column indexes
use the register's local basis ordering, with local wire zero as the least
significant bit.

Register QFT and inverse QFT are decomposed into Hadamard, controlled-phase, and
wire-swap operations. Walsh-Hadamard applies a Hadamard to each wire in the
selected register. Grover diffusion reflects amplitudes in the selected
register around their mean and repeats that reflection the requested number of
times.

By default, each executed operation is printed to standard output. Pass
`false` as the third `simulator` constructor argument to suppress the operation
trace and measurement/sampling output. Gate trace lines are printed after their
operation timer stops; measurement and sampling output is printed inside its
timed operation interval. Console output can take longer than numerical gate
execution.

## Measurement and sampling

Measurement and sampling are observations of a copy of the current state, not
state-changing quantum operations in this simulator. The gate timeline continues
from the original vector after an observation, and subsequent operations use
that unchanged vector. This is deliberate: these operations do not project or
renormalize the simulator's state.

```cpp
operations.measure(circuit[0], 0);
operations.sampler(circuit[1], 1024);
```

At their position in the timeline, `measure` sums the probabilities for the
selected wire being one, then draws one Bernoulli sample. It prints, for
example:

```text
measure(). measured q[0] = 1
```

`sampler` computes the same one-wire probability and uses a binomial
distribution to draw the number of ones in the requested number of shots. It
prints a two-outcome histogram:

```text
sampler() -> sampled q[1] | shots = 1024 || |0> 516 | |1> 508
```

The binomial draw is equivalent to independent repeated measurements of that
wire for the reported marginal counts; it avoids drawing and storing every
individual shot. The random generator is seeded from `std::random_device` for
each simulator run. Neither observation changes the state vector returned by
`getStateVector()`.

The register overloads observe a joint register basis outcome. `measure(reg)`
prints its bit string and decimal index once. `sampler(reg, shots)` draws one
joint outcome per shot and prints the nonzero histogram entries; unlike
independent per-wire samplers, this preserves correlations within the register.

The one-wire sampler API samples one wire at a time, so separate one-wire
histograms do not preserve correlations between wires. Use `sampler(reg, shots)`
when joint outcomes for a complete register are needed.

## Utility helpers

`utils/quantum_helpers.hpp` provides optional header-only helpers without
adding algorithm routines to the core operators. `unrollQuantumPhaseEstimation`
appends Hadamards, callback-supplied controlled powers \(U^{2^k}\), and an
inverse QFT to a caller-owned timeline. The caller supplies circuit register
views and a callback that receives the current phase-wire view, target-register
view, and power. The target register must already contain an eigenstate of the
unitary implemented by the callback.

`calculateRegisterProbabilities` marginalizes a state vector onto a selected
register. `deducePeriodFromPhaseDistribution` samples those phase probabilities
and searches their continued-fraction convergents for a candidate period.
Classical helpers such as `gcd`, modular arithmetic, modular multiplication
matrices, and continued-fraction convergents are provided inline by
`utils/math_helpers.hpp`.

For custom register matrices, `applyRegister` requires a finite unitary matrix
of size \(2^r \times 2^r\). `applyControlledRegister` accepts the same target
matrix plus one control wire outside that target register.

The last recorded execution time is available from `simulator.duration()`. It
sums the per-operation execution intervals, including measurement and sampling
work. It excludes state initialization, timeline validation, and gate-trace
printing; measurement and sampling output is included unless output is
suppressed.

## Variational circuits

The simulator is the execution layer used by the variational module. An
`ansatz` derives from `circuit` and generates a gate timeline from a parameter
vector; Hamiltonians and generic state-vector cost functions are separate
objects in `quantum::costfxn`. The optimizer runs repeated quiet simulations
to minimize these objectives using Adam and centered finite differences.

See the architecture guides for the [variational API](variational.md)
and [QNN pipeline](qnn.md). Algorithm examples are not compiled into
the application; the [manual](../manual.md) shows how to build algorithm circuits
from the engine primitives.
