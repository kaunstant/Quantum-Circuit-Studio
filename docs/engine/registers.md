# Qubits and Registers

The engine distinguishes the circuit's global wire layout from register blocks.
`qubit` is currently a minimal marker type. `registers::q_register` is a
fixed-width container of those markers; appending it to a `quantum::circuit`
assigns the register a consecutive range of global wire indices.

## Register creation and indexing

```cpp
#include "circuit.hpp"
#include "q_register.hpp"

quantum::circuit circuit;
registers::q_register phase(4);
registers::q_register work(3);
circuit.append(phase); // register ID 0, wires q0..q3
circuit.append(work);  // register ID 1, wires q4..q6

const auto phaseView = circuit(0);
const auto workView = circuit(1);
const auto control = phaseView[2]; // local wire 2, global wire q2
```

Register IDs are assigned in register-append order starting at zero. Global
wire indices include both standalone wires and register wires. The circuit
keeps components in append order, so each register occupies a consecutive
global interval.

The `q_register` type exposes its width and concatenation through `operator+`
and `operator+=`. Direct qubit subscripting on `q_register` is not part of the
active API; obtain `register_view` values from the owning circuit when building
operations. The view exposes its size, start index, register ID, context, and
local-wire subscript.

## Register-level operations

Register views are accepted by transforms, sampling, and custom-register
matrices:

```cpp
quantum::operators operations;
operations.walshHadamard(phaseView);
operations.qft(phaseView);
operations.sampler(phaseView, 1024);
```

`sampler(registerView, shots)` samples the joint basis distribution of that
register. It is not equivalent to independent samplers on each wire, since the
joint outcome histogram retains correlations between wires.

When `applyRegister` is used, an \(r\)-wire register needs a unitary of dimension
\(2^r\). The matrix acts in register-local basis order. `applyControlledRegister`
adds one control wire outside the selected register. The simulator validates
the target width, control, matrix shape, finite values, and unitarity before
execution.

## Circuit views and lifetime

`circuit[index]` returns a `qubit_view`; `circuit(registerId)` returns a
`register_view`. These views are non-owning and retain a reference to their
source circuit. Keep that circuit alive while using its views or the
view-based operations created from them. The simulator validates that
view-stamped operations belong to the same circuit passed to its constructor.

## Classical registers

`c_register.hpp` is currently an empty placeholder. The simulator's
single-wire measurement API accepts a classical-bit index as operation
metadata, but prints the sampled value rather than writing to a persistent
classical register or returning a measurement-result object.

## Practical limits

Registers organize wires and provide local indexing; they do not reduce the
simulator's state-vector size. A circuit with \(n\) wires still requires
\(2^n\) complex amplitudes, and full-register matrices grow as \(2^r \times
2^r\). Dense state and matrix storage therefore become the limiting factor
quickly as wire count increases.

## Related guides

- [Circuit layout and views](circuit.md)
- [Register operations](operators.md)
- [State-vector limits](simulator.md)
- [Register operator example](../../example/canvas/other_operators/CustomOperators.cpp)
