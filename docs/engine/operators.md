# Operators and Operation Timelines

`quantum::operators` is the engine's instruction builder. Each call appends
metadata for one operation to an ordered timeline; the call itself does not
change a state vector. A `quantum::simulator` later validates and executes those
operations against a `quantum::circuit`.

## Recording gates

Operations support either integer wire indices or views created from a circuit:

```cpp
quantum::circuit circuit;
circuit.append(registers::q_register(2));
const auto reg = circuit(0);

quantum::operators operations;
operations.h(reg[0]);
operations.ry(reg[1], 0.25);
operations.cx(reg[1], reg[0]); // first argument is target, second is control
```

The available standard gates include Pauli X/Y/Z, Hadamard, T, rotations
Rx/Ry/Rz, controlled X/Y/Z, SWAP, and Toffoli (CCX). Rotations take angles in
radians. The two-wire gate API consistently names the target first and control
second; `swap` takes two peer wires.

The timeline preserves append order. Views stamp the originating circuit onto
an operation, allowing the simulator to detect use of a view from a different
circuit. Integer-index operations are checked for valid indices at simulation
time but cannot carry that origin check.

## Custom matrices

`apply`, `controlled`, and `multi_controlled` accept concrete custom
single-qubit matrices:

```cpp
quantum::Matrix2D xMatrix = {
    {{0.0, 0.0}, {1.0, 0.0}},
    {{1.0, 0.0}, {0.0, 0.0}}
};
operations.apply(reg[0], xMatrix);
operations.controlled(reg[1], reg[0], xMatrix);
operations.multi_controlled(reg[0], {2, 3}, xMatrix);

quantum::Matrix2D registerUnitary = /* 4-by-4 unitary matrix */;
operations.applyRegister(reg, registerUnitary);
operations.applyControlledRegister(reg, circuit[2], registerUnitary);
```

Custom single-wire matrices must be finite and unitary with shape \(2\times2\).
Their timeline entries contain the matrix itself, not a separate parameter
vector. For parameter-dependent custom gates, calculate the matrix from the
current user-owned parameter values before appending the operation. Optimizers
and parameter updates remain outside the operator timeline.

For an \(r\)-wire register, the matrix must be \(2^r \times 2^r\), square,
finite, and unitary. `applyControlledRegister` has one control wire, which must
not overlap the target register. The simulator checks these requirements when
`run()` begins; invalid metadata raises
`quantum::circuit_operation_mismatch`.

The explicit matrix uses register-local basis ordering: local wire zero is the
least-significant bit. The matrix is not interpreted as an arbitrary
non-unitary transform.

The custom-matrix overloads accept the matrix only. To optimize a custom gate,
keep its parameters in caller-owned state, construct the corresponding matrix
for the current parameter values, and append that matrix to the timeline. The
operator timeline does not retain or update custom-gate parameters.

## Register transforms

The timeline can also contain built-in register-level instructions:

| Operation | Effect |
|---|---|
| `qft(reg)` | Applies the quantum Fourier transform to the selected register |
| `iqft(reg)` | Applies its inverse |
| `walshHadamard(reg)` | Applies Hadamard to every wire in the register |
| `amplitudeAmplification(reg, iterations)` | Applies the register's Grover diffusion reflection the requested number of times |

These calls append engine primitives; they do not add algorithm-specific
oracles. A Grover search still needs an oracle circuit assembled from gates
before the diffusion step.

## Measurement and sampling

`measure(qubit, classicalBit)` records a one-shot observation; register
measurement reports a joint register outcome. `sampler(wire, shots)` and
`sampler(reg, shots)` report repeated sampling. The register sampler preserves
joint correlations between its wires; separate one-wire samplers do not.

In the current simulator, these observations sample a copy of the current state
and do not collapse or modify the state used by later timeline operations.
They print the results to standard output rather than storing a classical
register result object. Shot counts must be positive.

## Barriers and inspection

`barrier()` or `barrier(wires)` records a no-op scheduling marker. The simulator
currently skips barriers during state evolution. `getGates()` exposes the
timeline as a const vector of `quantum::gate` metadata; it is useful for
inspection and drawing, but it does not provide a rendered circuit diagram.

## Validation and failure behavior

Most structural validation happens at the start of `simulator.run()`. The
simulator checks target/control ranges, distinctness, operation metadata,
parameter finiteness, register ownership, and matrix dimensions/unitarity.
Operator creation methods that require incompatible circuit views can also
throw immediately. See [the simulator guide](simulator.md) for the checks
performed per operation type.

## Related guides

- [Circuit layout](circuit.md)
- [Quantum registers](registers.md)
- [Simulation and timing](simulator.md)
- [Variational optimization](variational.md)
- [Custom and register operators example](../../example/canvas/other_operators/CustomOperators.cpp)
