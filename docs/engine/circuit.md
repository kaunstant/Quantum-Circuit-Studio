# Circuit Layout and Wire Views

`quantum::circuit` describes the ordered wire layout used by a simulation. It
owns the circuit's structural components; it does not own the operations. Gates
are recorded separately in a `quantum::operators` timeline and executed together
with the circuit by `quantum::simulator`.

## Building a circuit

A circuit can begin with standalone qubits and then append more standalone
qubits or quantum registers:

```cpp
#include "circuit.hpp"
#include "q_register.hpp"

quantum::circuit circuit(1);          // Standalone wire q0
registers::q_register data(3);
circuit.append(data);                 // Register reg0: q1, q2, q3
registers::q_register work(2);
circuit.append(work);                 // Register reg1: q4, q5

const std::size_t wires = circuit.numQubits();       // 6
const std::size_t blocks = circuit.numRegisters();   // 2
```

Components are laid out in append order. A register occupies one consecutive
block of global wires, but register IDs count only register components;
standalone qubits do not consume register IDs. Adding a register advances both
the total wire count and the register count.

`operator+=` and `operator+` append another circuit's components in their
existing order. These operations copy the components into the destination
circuit; they do not merge gate timelines.

## Global wires and register views

`circuit[index]` creates a read-only `quantum::qubit_view` for a global wire.
`circuit(registerId)` creates a `quantum::register_view`. A register view can
then address its local wires:

```cpp
const auto phase = circuit(0);
const auto firstPhaseWire = phase[0];

const std::size_t globalIndex = firstPhaseWire.globalIdx();
const std::size_t localIndex = firstPhaseWire.localIdx();
const std::size_t firstWire = phase.startIndex();
```

A wire view reports its global index, whether it belongs to a register, and,
when applicable, its register ID and local index. A register view reports its
ID, width, starting global wire index, and circuit context. Views refer to the
circuit that created them; that circuit must outlive the views and any
view-based operations recorded from them.

View overloads are useful because the resulting operation remembers its source
circuit. At `simulator.run()`, the simulator rejects a view-based operation
created from another circuit. Integer-index overloads do not carry that origin
identity, so they are validated against the bound circuit's ranges instead.

## Construction and execution are separate

The circuit is a layout, not a circuit builder that contains gates. Create the
layout first, obtain views from it, append operations to a timeline, and then
run:

```cpp
#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

quantum::circuit circuit;
circuit.append(registers::q_register(2));

quantum::operators operations;
operations.h(circuit(0)[0]);
operations.cx(circuit(0)[1], circuit(0)[0]);

quantum::simulator simulator(circuit, operations, false);
simulator.run();
```

The simulator stores non-owning references to the circuit and operation
timeline. Both objects must remain alive and unchanged while the simulator uses
them. Each `run()` validates the full timeline and starts from
\(|0\ldots0\rangle\).

## Current boundaries

- `circuit` currently models ordered standalone-qubit and quantum-register
  components. The classical-register header is not wired into measurement
  results; measurements and samplers currently print observations.
- Circuit combination copies layout components only. Build or retain the
  corresponding operation timeline separately.
- Views are metadata handles, not owning qubits or registers.
- State-vector size and simulator memory grow exponentially with total circuit
  wires. See [the simulator guide](simulator.md) for execution constraints
  and validation details.

## Related guides

- [Operators and timelines](operators.md)
- [Quantum registers](registers.md)
- [Simulator behavior](simulator.md)
- [QML pipeline](qnn.md)
- [Circuit canvas example](../../example/canvas/elementary/BellState.cpp)
