# Performance and Complexity

## 1. Scope

This document describes the asymptotic cost of the v0.3.5 CPU statevector implementation. Let `n` be the number of circuit qubits and `N = 2^n` the statevector length. Let `k` be the number of qubits addressed by a supplied matrix operator, and let `p` be the number of trainable parameters.

## 2. Engine limits

The configured default limit is 32 qubits (`Config::DEFAULT_MAX_QUBITS`). A statevector has `2^32` amplitudes at that limit, requiring approximately 64 GiB for double-precision complex values before temporary buffers. Practical limits are normally much lower and depend on available memory.

Classical register storage defaults to 32 bits. Matrix operators are intended for small `k`; a dense `2^k x 2^k` matrix requires `O(4^k)` complex entries.

## 3. Statevector memory

State initialization stores `N` complex amplitudes:

```text
Time:   O(N)
Memory: O(N)
```

The matrix application kernel makes a copy of the state, so a supplied matrix operation can temporarily require another `O(N)` amplitudes.

## 4. Single-qubit gates

X, Y, Z, H, T, RX, RY, and RZ process each pair of amplitudes differing at the target bit:

```text
Time:   O(N)
Extra memory: O(1)
```

They do not construct an `N x N` system matrix.

## 5. Controlled built-in gates

CX, CY, and CZ scan the state and process only target-zero pairs whose control bit is one:

```text
Time:   O(N)
Extra memory: O(1)
```

SWAP and CCX use the same indexed state traversal and are also `O(N)`.

## 6. Multi-controlled X

A multi-controlled X checks each of `c` control bits for each basis index:

```text
Time:   O(cN), usually O(N) for a fixed circuit gate
Extra memory: O(1)
```

The implementation avoids expanding the operation into a full matrix.

## 7. Dense matrix operators

For `k` target qubits, the simulator validates a dense matrix of dimension `D = 2^k`, copies the complete state, and computes each target subspace contribution:

```text
Matrix validation: O(D^2)
State application: O(ND)
Matrix storage:    O(D^2)
Temporary state:   O(N)
```

The matrix API is therefore suitable for small operators. Large operators should use a dedicated structured gate or future sparse/tensor backend.

## 8. Controlled dense operators

A controlled dense operator has the same asymptotic cost as its target operator, with work skipped for control-zero basis states:

```text
Time:   O(ND)
Memory: O(N + D^2)
```

## 9. Measurement

A single measurement samples by scanning all basis probabilities, then collapses and renormalizes the state:

```text
Probability scan: O(N)
Collapse:         O(N)
Memory:           O(N) for the probability vector in the convenience path
```

The backend computes a probability as `norm(amplitude)`, samples a basis index, reads the requested bit, zeros incompatible amplitudes, and divides surviving amplitudes by the square root of the surviving probability.

## 10. Probability calculation

`QuantumSimulator::probabilities` computes one norm per amplitude:

```text
Time:   O(N)
Memory: O(N)
```

## 11. Circuit execution

For `g` operations, execution is approximately:

```text
O(gN)
```

for built-in gates, with dense matrix operations contributing `O(ND)` each.

## 12. QFT

The circuit-library QFT contains `O(n^2)` controlled phases and Hadamards plus `O(n)` swaps. Since each built-in operation scans the statevector:

```text
Circuit construction: O(n^2)
Simulation:           O(n^2 N)
```

This is the standard statevector cost of simulating a gate decomposition; it is not the `O(n^2)` cost of an optimized classical FFT over amplitudes.

## 13. Inverse QFT

Inverse QFT has the same operation count and complexity:

```text
Circuit construction: O(n^2)
Simulation:           O(n^2 N)
```

## 14. Grover search

For search space size `N`, the standard iteration count is approximately `pi * sqrt(N) / 4`. Each iteration contains `O(n)` gates in this implementation:

```text
Circuit construction: O(n sqrt(N))
Simulation:           O(nN sqrt(N))
Memory:               O(N)
```

The reusable API is intended for small demonstrations because statevector simulation removes the asymptotic hardware advantage.

## 15. Shor arithmetic and period finding

The small reference implementation constructs dense permutation matrices over a work register of `w` qubits:

```text
Matrix storage per modular operation: O(4^w)
Matrix application:                  O(N 2^w)
```

The number of controlled modular operations is proportional to the counting-register width. This is educational and correct for small examples such as 15, not a scalable factoring backend.

## 16. Pauli Hamiltonian expectation

For `t` Pauli terms over `n` qubits, each term visits all `N` amplitudes and performs bit-mask work:

```text
Time:   O(tN)
Memory: O(1) extra beyond the state
```

## 17. VQE

A VQE energy evaluation includes ansatz construction, statevector execution, and Hamiltonian expectation:

```text
One energy evaluation: O(gN + tN)
```

With coordinate descent, each iteration evaluates up to two trials per parameter:

```text
O(i p (gN + tN))
```

where `i` is the optimizer iteration count.

## 18. QAOA

For a graph with `m` edges and `l` parameter layers, the circuit has `O(l(m+n))` operations. Its statevector cost is:

```text
Circuit execution: O(l(m+n)N)
Cost evaluation:   O(mN)
```

Coordinate-descent optimization adds the same `O(i p)` trial factor as VQE.

## 19. Parameter-shift gradients

The parameter-shift rule evaluates the objective twice per parameter:

```text
One gradient: O(2p C)
```

where `C` is the cost of one forward objective evaluation. For an expectation-output QNN, `C` includes statevector execution and observable evaluation.

## 20. QNN forward pass

A QNN forward pass builds and executes a feature-map-plus-ansatz circuit:

```text
Time:   O(gN + outputCost)
Memory: O(N)
```

Expectation output with `t` observables has `outputCost = tN`. A probability output for `r` selected qubits scans the state `r` times, or `O(rN)` with the current simple implementation.

## 21. QNN backward pass and training

A backward pass uses parameter shift:

```text
Time: O(2p C)
```

For `s` training samples and `i` gradient-descent iterations:

```text
Training: O(i s 2p C)
```

This is intentionally transparent reference code. Batched statevector execution, caching, parallel evaluation, automatic differentiation, and hardware shot management are future optimization areas.

## 22. Practical guidance

- Keep `n` small when using dense matrix operations.
- Prefer built-in gates for `O(N)` kernels.
- Use one or a few observables per QNN output unless a larger vector is necessary.
- Treat 32 qubits as a configuration ceiling, not a promise of practical simulation at that size.
- Profile circuit depth and parameter count before increasing training iterations.
- A future Qt frontend should display timing and memory estimates without moving those concerns into the simulation kernels.
