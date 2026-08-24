# Quantum Circuit Simulation — Technical Manual

## 1. Purpose

This document describes the internal design of the v0.1 quantum circuit simulation engine.

The goal of v0.1 is to establish a correct and understandable CPU statevector simulator that can later serve as the reference implementation for optimized backends.

---

## 2. High-Level Architecture

The current implementation has two principal components:

```text
QuantumCircuit
      │
      │ ordered vector of Gate objects
      ▼
QuantumSimulator
      │
      │ applies gates to statevector
      ▼
std::vector<std::complex<double>>
```

### QuantumCircuit

`QuantumCircuit` describes **what operations should be performed**.

It stores:

- number of qubits
- ordered gate operations

It does not need to know how a quantum gate modifies the statevector.

### QuantumSimulator

`QuantumSimulator` describes **how the circuit is executed**.

It:

1. receives a circuit
2. initializes/operates on a statevector
3. traverses the stored gates in order
4. applies the corresponding gate operation
5. returns or exposes the resulting state

This separation allows the simulator implementation to evolve without changing the circuit representation.

---

## 3. Statevector Representation

For `n` qubits, the statevector contains:

```text
2^n
```

complex amplitudes.

Conceptually:

$$
|\psi\rangle = \sum_{i=0}^{2n-1} \alpha_i |i\rangle
$$


where `alpha_i` is the complex probability amplitude associated with basis state `|i>`.

The implementation stores these amplitudes in a contiguous array such as:

```cpp
std::vector<std::complex<double>> state;
```

The initial computational-basis state is normally:

```text
|00...0>
```

which corresponds to:

```text
state[0] = 1
```

and all other amplitudes equal to zero.

---

## 4. Basis-State Indexing

Each integer index corresponds to the binary representation of a computational basis state.

For three qubits:

```text
Index    Binary basis state
0        |000>
1        |001>
2        |010>
3        |011>
4        |100>
5        |101>
6        |110>
7        |111>
```

Therefore:

```cpp
state[5]
```

corresponds to the amplitude of:

```text
|101>
```

The exact left-to-right qubit labeling convention must remain consistent throughout the entire engine.

---

## 5. Why Full Gate Matrices Are Avoided

A direct matrix implementation would represent an `n`-qubit operation using a matrix of dimension:

```text
2^n × 2^n
```

This becomes impractical very quickly.

The statevector itself only requires:

```text
2^n
```

complex numbers.

The v0.1 implementation therefore applies gates directly to groups of amplitudes using bit operations rather than explicitly constructing full system matrices.

---

## 6. Gate Representation

A gate is represented by a `GateType` plus qubit and parameter information.

The conceptual representation is:

```cpp
struct Gate
{
    GateType type;

    int qubit1 = -1; // target
    int qubit2 = -1; // control
    int qubit3 = -1; // control

    double theta = 0.0;
};
```

The semantic convention used:

```text
qubit1   → target qubit
qubit2   → first control qubit
qubit3   → second control qubit

theta    → rotation angle for parameterized gates
```

---

## 7. Circuit Storage

The circuit stores operations in execution order:

```cpp
std::vector<Gate> m_gates;
```

For example:

```cpp
circuit.H(0);
circuit.CX(1, 0);
circuit.RZ(1, theta);
```

conceptually becomes:

```text
Gate 0 → H(target=0)
Gate 1 → CX(target=1, control=0)
Gate 2 → RZ(target=1, theta)
```

The simulator then traverses this vector in order.

This is important because quantum gates generally do not commute.

---

# 8. Bit Manipulation

For qubit `q`, its position in a basis-state index can be selected with:

```cpp
int bit = 1 << q;
```

A bit is set when:

```cpp
(index & bit) != 0
```

and clear when:

```cpp
(index & bit) == 0
```

To toggle a qubit:

```cpp
index ^ bit
```

To set a qubit:

```cpp
index | bit
```

These operations allow gate kernels to locate the relevant amplitudes without constructing large matrices.

---

### 9. Single-Qubit Gate Pattern

For a single-qubit gate, amplitudes can be processed in pairs. 

For target qubit q: 

```text
i       → target bit = 0
j=i|bit → target bit = 1
```

The pair: 

```text
state[i]
state[j]
```

represents the two target-qubit components on the same configuration of all other qubits. 

### Example (Target Qubit q₁)

For a 4-qubit system (q₃ q₂ q₁ q₀) targeting **q₁** (the second bit from the right): 

* **State i (1101)**: Target bit q₁ is 0.
* **State j (1111)**: Target bit q₁ is 1.

All other qubits (q₃, q₂, q₀) stay fixed at 1, 1, and 1. 

A 2×2 gate matrix can therefore be applied directly to the pair: 

```text
state[1101]
state[1111]
```


## 10. X Gate

The Pauli-X gate performs:

```text
|0> → |1>
|1> → |0>
```

For each target pair, the amplitudes are swapped:

```cpp
std::swap(state[i], state[j]);
```

This can be implemented without constructing the full system matrix.

---

## 11. Y Gate

The Pauli-Y matrix is:

```text
[ 0  -i ]
[ i   0 ]
```

For a pair `(v0, v1)`:

```text
new_v0 = -i v1
new_v1 =  i v0
```

The original pair must be copied before overwriting either amplitude.

---

## 12. Z Gate

The Pauli-Z gate is diagonal:

```text
|0> → |0>
|1> → -|1>
```

Therefore only amplitudes whose target bit is set need to be negated.

---

## 13. Hadamard Gate

The Hadamard operation is:

```text
H = 1/sqrt(2) [ 1   1 ]
              [ 1  -1 ]
```

For each target pair:

```text
new_v0 = (v0 + v1) / sqrt(2)
new_v1 = (v0 - v1) / sqrt(2)
```

This is the primary source of superposition in basic circuits.

---

## 14. Controlled Gates

A controlled gate applies an operation to the target only when the control qubit is in state `1`.

For a control bit:

```cpp
int controlBit = 1 << control;
```

the control condition is:

```cpp
(index & controlBit) != 0
```

For CX, the target operation is an X operation.

---

## 15. CX / CNOT

CX applies X to the target when the control is `1`.

For the relevant amplitude pair:

```text
control = 1
target  = 0
```

the partner index is obtained by setting the target bit:

```cpp
int j = i | targetBit;
```

and the amplitudes are swapped.

Processing only the target-bit-zero half avoids performing the same swap twice.

---

## 16. CY

CY applies Y to the target when the control is `1`.

For:

```text
v0 = state[i]
v1 = state[j]
```

the transformation is:

```text
new_v0 = -i v1
new_v1 =  i v0
```

where `i` here denotes the imaginary unit.

---

## 17. CZ

CZ applies Z to the target when both the control and target bits are `1`.

Therefore:

```cpp
state[index] = -state[index];
```

is sufficient for the affected amplitudes.

---

## 18. SWAP

SWAP exchanges two qubit positions.

A pair of basis indices is selected where:

```text
qubit1 = 0
qubit2 = 1
```

and the corresponding partner has:

```text
qubit1 = 1
qubit2 = 0
```

The amplitudes are then swapped.

Only one orientation of each pair should be processed to avoid swapping twice.

---

## 19. CCX / Toffoli

CCX applies X to the target only when both controls are `1`.

The conditions are:

```cpp
(index & targetBit) == 0     // apply x gate on target qubit if
(index & control1Bit) != 0   // control qubit 1 = |1>
(index & control2Bit) != 0   // control qubit 2 = |1>
```

The target bit is then set to obtain the partner index and the amplitudes are swapped.

---

## 20. Parameterized Rotation Gates

RX, RY, and RZ use an angle `theta`.

The simulator should apply the corresponding 2×2 unitary directly to each target-qubit amplitude pair.

For example:

$$
R_X(\theta)
=
\begin{bmatrix}
\cos(\theta/2) & -i\sin(\theta/2)\\
-i\sin(\theta/2) & \cos(\theta/2)
\end{bmatrix}
$$

The implementation should avoid constructing a full `2^n × 2^n` matrix.

---

# 21. Complexity

The statevector contains:

```text
O(2^n)
```

complex amplitudes.

A typical single-qubit or two-qubit gate therefore requires work proportional to:

```text
O(2^n)
```

rather than performing a full matrix-vector multiplication involving a `2^n × 2^n` matrix.

Memory usage remains exponential in the number of qubits, however.

For double-precision complex amplitudes, one amplitude requires approximately 16 bytes.

Therefore the statevector alone requires approximately:

```text
16 × 2^n bytes
```

before accounting for additional temporary memory.

---

# 22. Validation Circuits

Useful early validation circuits include:

### Bell state

```cpp
circuit.H(0);
circuit.CX(1, 0);
```

Expected state:

```text
(|00> + |11>) / sqrt(2)
```

### GHZ state

For three qubits:

```cpp
circuit.H(0);
circuit.CX(1, 0);
circuit.CX(2, 1);
```

Expected state:

```text
(|000> + |111>) / sqrt(2)
```

These are useful sanity checks for statevector and controlled-gate implementations.

---
