# Quantum Circuit Studio

Quantum Circuit Studio (QCS) is a C++17 state-vector simulator with a Qt
desktop frontend for viewing quantum circuits. The project combines a
standalone simulation engine with a circuit-canvas application and source
examples built around the engine's public API.

## Version 0.4

This version introduces a Qt frontend with a circuit canvas, substantial
backend changes, and updated API naming conventions.

## Highlights

- Dense complex state-vector simulation for small circuits and experiments.
- Ordered circuit layouts, quantum registers, and gate-operation timelines.
- Standard gates, parameterized rotations, controlled gates, custom unitary
  matrices, barriers, and register-level operations.
- Measurement and sampling for individual wires and joint register outcomes.
- QFT, inverse QFT, Walsh-Hadamard, and amplitude-amplification primitives.
- Simulator-side checks for operation ranges and metadata, matrix dimensions,
  finite values, and unitarity.
- Classical helpers for GCD, modular arithmetic, continued fractions, and
  phase-estimation post-processing.
- Variational circuits, Hamiltonian expectations, and Adam-based VQE and
  custom-objective optimization.
- QNN feature maps, expectation/probability readout, losses, gradients,
  single-threaded mini-batch training, metrics, regression preparation helpers,
  and model save/load.
- A Qt circuit canvas for drawing circuit layouts and operation timelines.

The simulator is an ideal, CPU-based dense state-vector implementation. An
\(n\)-wire state requires \(2^n\) complex amplitudes, so memory and runtime grow
exponentially. It is intended for learning, small-scale experimentation, and
prototyping; it does not simulate noise or execute circuits on quantum
hardware.

Measurement and sampling report observations without collapsing or changing
the state used by subsequent timeline operations. The optimizer and QNN
training paths run on a single thread.

## Project layout

```text
app/
  include/                  Qt circuit-canvas interface
  src/                      Qt circuit-canvas implementation
engine/
  include/
    core/                   Circuit, operation timeline, simulator
    registers/              Quantum register types
    utils/                   CSV, quantum, and math helpers
    variational/             Ansatz and optimizer interfaces
    qml/                    QNN, gradients, losses, and data helpers
  src/                      Engine implementations
example/                     Standalone algorithm and canvas examples
benchmarks/
  qcs/                       C++ benchmark snippets for manual use
  qiskit/                    Python benchmark runner and requirements
docs/
  manual.md                  User guide and benchmark results
main.cpp                     Qt application entry point
run.bat                      Windows configure, build, and launch script
```

Files under `example/` are reference programs and are not compiled into the
application. To try one, use its code as the application entry point or build
it separately against the engine. The benchmark examples under `benchmarks/`
are also intentionally outside the application build.

## Requirements

- Windows 10 or later, 64-bit.
- Visual Studio 2019/2022 or Build Tools with the **Desktop development with
  C++** workload and MSVC.
- CMake 3.25 or later.
- The project's local Qt 6 files under `.tools/Qt`.

The CMake project uses the local Qt distribution rather than searching for a
system Qt installation. Keep `.tools/Qt` included in the GitHub repository so
cloning the project includes the Qt headers, libraries, runtime DLLs, and
platform plugin required to configure, build, and run the frontend. The
project's `.gitignore` deliberately does not exclude this directory. Retain the
Qt license and notices associated with the included distribution.

The expected local Qt layout includes `include/`, `lib/`, and `bin/`, with the
Windows platform plugin under `bin/plugins/platforms/`. Debug builds also use
the Qt debug libraries and plugin.

## Build and run on Windows

From the project root, run:

```bat
run.bat debug
```

For an optimized build, use:

```bat
run.bat release
```

If the build type is omitted, the script prompts for Debug or Release. The
launcher initializes the installed MSVC environment, uses CMake from `PATH` or
downloads its configured portable CMake version when necessary, then
configures, builds, and launches the application. Build files and installed
artifacts are written under `build/`.

The application uses Qt Widgets to display the configured circuit/model view.
The current `main.cpp` loads a compatible QNN archive from the path configured
there; save a model to that path before launching, or update the path for your
own model. The application reports a load error if the archive is missing or
incompatible.
The canvas API can also be used directly by constructing an
`app::circuit_canvas` with a circuit layout and its operation timeline, then
calling `draw()`, `show()`, and `QApplication::exec()`. See the
[canvas examples](example/canvas/).

## Using the engine

The engine keeps circuit layout, operations, and execution separate:

```cpp
#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

quantum::circuit circuit;
circuit.append(registers::q_register(2));

quantum::operators operations;
operations.h(circuit(0)[0]);
operations.cx(circuit(0)[1], circuit(0)[0]); // target first, control second
operations.sampler(circuit(0), 1024);

quantum::simulator simulator(circuit, operations);
simulator.run();
```

Operations are appended to a timeline and executed in order by `run()`. Circuit
views are tied to their source circuit; keep the circuit and operations alive
while using the simulator or canvas. Register-local wire zero is the
least-significant bit in its basis index. The simulator prints operation and
measurement output by default; pass `false` to the simulator constructor to
suppress it.

For detailed API and behavior information, see the
[user manual](docs/manual.md) and the [engine guides](docs/engine/).

## Examples and benchmarks

The examples demonstrate elementary circuits, Grover search, QFT and phase
estimation, Shor-style order finding, VQE, QAOA, QNNs, and circuit-canvas use.
Each example is a standalone source file with its own `main()` and is kept
outside the application target.

The [benchmark guide](benchmarks/README.md) explains how to run the matching
QCS and Qiskit workloads. The measured results and workload settings are
recorded in the [manual](docs/manual.md). Benchmark figures describe those
specific implementations and test conditions; they are not general
performance guarantees.
