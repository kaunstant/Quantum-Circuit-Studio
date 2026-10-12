# Simulator benchmarks

This folder contains matched QCS and Qiskit workloads for every row in the
benchmark table in `docs/manual.md`. Each run prints one end-to-end wall-clock
time in milliseconds. Repeat each workload 16 times and calculate the mean
yourself; do not treat one run as an average.

The timed boundary starts immediately before workload setup and ends after
simulation and required classical post-processing. For Qiskit shot-based
workloads, backend transpilation is included in the timed boundary. Imports,
Python startup, and terminal output are excluded. QNN timing includes CSV
loading, feature scaling, training, prediction, and metric calculation. The
fixed workload settings are printed beside the result.

## Qiskit

Create and activate the environment once, then install the pinned dependencies:

```powershell
py -3 -m venv benchmarks\qiskit\.venv
benchmarks\qiskit\.venv\Scripts\Activate.ps1
python -m pip install -r benchmarks\qiskit\requirements.txt
```

Run one workload from the repository root:

```powershell
python benchmarks\qiskit\benchmark.py bell
```

Accepted workload names: `bell`, `grover`, `qft`, `phase-estimation`,
`shor`, `vqe`, `qaoa`, `qnn-classification`, and `qnn-regression`.
Each invocation performs exactly one timed run.

## QCS

The QCS files are standalone C++ examples and are intentionally not added to
CMake. Copy the contents of one file under `qcs/` into `main.cpp`, build and
run the project as usual, then restore `main.cpp` before trying another
workload. The snippets report one end-to-end wall time and the simulator's
internal simulation duration. Record the end-to-end wall time in the table.

The QNN examples use a small deterministic subset of the existing wine dataset
so they can be repeated without changing or splitting source CSV files. All
QNN benchmark dataset sizes and epoch/batch settings are printed by the
programs.
