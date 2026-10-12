# Quantum Neural Networks

The QNN module composes three existing parts of the engine:

1. A **feature map** converts one classical input row into quantum gates.
2. A **variational ansatz** adds trainable gates using a weight vector.
3. A **readout** converts the simulated state into an expectation value or a
   measurement probability.

Losses, gradients, and training build on this forward path. The QNN is a
pipeline manager, not a `circuit` or `ansatz` subclass: it composes those
structures for each evaluation.

## Constructing a model

The constructor takes the number of wires, a feature-map callback, and an
ansatz blueprint. Both callbacks append operations to the same timeline:

```cpp
#include "qnn.hpp"

#include <vector>

constexpr std::size_t numQubits = 2;

quantum::variational::ansatz ansatz(
    numQubits,
    2,
    [](quantum::operators& ops, const std::vector<double>& weights) {
        ops.ry(0, weights.at(0));
        ops.ry(1, weights.at(1));
    });

quantum::ml::qnn model(
    numQubits,
    [](quantum::operators& ops, const std::vector<double>& features) {
        ops.rx(0, features.at(0));
        ops.rx(1, features.at(1));
    },
    ansatz);
```

The feature map runs first, then the ansatz. The feature vector and weight vector are passed into each evaluation. The
explicit-weight overloads remain available for optimization workflows. A
successful `train` call also retains the returned weights and final loss in
the model for inference and persistence.

## Forward evaluation and readout

The default output is the expectation of Pauli Z on wire 0. This gives a value
in \([-1, 1]\), suitable for regression-style outputs:

```cpp
const std::vector<double> input = {0.4, 0.8};
const std::vector<double> weights = {0.2, -0.1};

const double prediction = model.forward(input, weights);
const std::vector<std::complex<double>> state =
    model.forwardStateVector(input, weights);
```

For binary classification, configure probability output. The output then
represents the probability of measuring the selected wire as one:

```cpp
model.setOutputMode(quantum::ml::OutputMode::Probability);
model.setOutputQubit(0);

const double classOneProbability = model.forward(input, weights);
const double sameProbability =
    model.forwardProbability(input, weights, 0);
```

`forwardBatch` evaluates each input row using the configured output mode.
`forwardValues` evaluates every configured Hamiltonian observable for one
input, regardless of the primary output mode. The default observable is Z on
wire 0; `setObservables` replaces it with a nonempty list of Hamiltonians,
each spanning all model wires.

## Losses and training

The model provides single-row and batch overloads of `loss`. With the default
`LossType::Automatic`, expectation readout selects mean-squared error and
probability readout selects binary cross-entropy. You can select a loss
explicitly when needed:

```cpp
const std::vector<std::vector<double>> inputs = {
    {0.1, 0.7},
    {0.8, 0.2}
};
const std::vector<double> targets = {1.0, 0.0};

const double currentLoss = model.loss(inputs, weights, targets);
```

Training receives initial weights and returns the optimized weights. It also
stores the resulting weights and final loss on the model:

```cpp
const std::vector<double> initialWeights = {0.2, -0.1};
const std::vector<double> trainedWeights =
    model.train(inputs, targets, initialWeights, 50, 0.05);

const std::vector<double> predictions =
    model.forwardBatch(inputs, trainedWeights);
const std::vector<double> storedWeightPredictions = model.forwardBatch(inputs);
```

The training implementation uses single-threaded Adam updates. The simple
overload uses the full dataset as one batch on each epoch. The mini-batch
overload visits every row once during each external epoch and performs one
optimizer update per batch:

```cpp
const std::size_t externalEpochCount = 10;
const std::size_t batchSize = 32;
const std::vector<double> trainedWeights =
    model.train(
        inputs,
        targets,
        initialWeights,
        externalEpochCount,
        batchSize,
        0.05);
```

The final batch may contain fewer than `batchSize` rows; training uses its
actual row count and does not read beyond the dataset. The no-weight `forward`,
`forwardBatch`, and
`getAssembledCircuit` overloads use the weights retained by training or loading;
they throw if the model has no stored trained weights. The overload that accepts a `TrainStepCallback`
additionally reports the epoch number, batch loss, and current weights:

```cpp
#include <iostream>

const std::vector<double> trained = model.train(
    inputs,
    targets,
    initialWeights,
    [](std::size_t epoch, double loss, const std::vector<double>& currentWeights) {
        std::cout << epoch << ": " << loss << '\n';
    },
    50,
    0.05);
```

The callback is invoked once after all batches in each external epoch have
completed. Callback loss is evaluated over the full training dataset.

The QNN training helper is a convenience optimizer. For custom optimizer
logic, call `loss` and `lossGradient` from a user-owned loop, or supply the
model's objective to another optimization routine.

## Backward gradients

`backward` differentiates the configured output; `backwardProbability`
differentiates the probability of measuring one on a specified wire.
`lossGradient` applies the selected loss's chain rule to those output
gradients:

```cpp
const std::vector<double> outputGradient =
    model.backward(input, weights);
const std::vector<double> objectiveGradient =
    model.lossGradient(input, weights, 1.0);
```

The standalone `parameterShiftGradient` evaluates parameter shifts in sequence:

```cpp
const std::vector<double> gradient =
    quantum::ml::parameterShiftGradient(
        objective, weights, 1.5707963267948966);
```

The standalone helpers in `gradients.hpp` provide centered finite differences
for general scalar functions and parameter-shift gradients for suitable quantum
rotation parameters. At the default \(\pi/2\) shift, parameter shift computes
\((f(\theta+\pi/2)-f(\theta-\pi/2))/2\). It is exact for the standard,
single-use rotation parameters in the current examples. It is not generally
exact if the same parameter is reused by several gates or if a parameter is
multiplied before being used as a gate angle; use finite differences or an
appropriate generalized rule for those circuits.

The functions in `losses.hpp` are separate vector utilities: MSE and binary
cross-entropy are in `quantum::ml::loss`; RMSE, R-squared, accuracy, precision,
recall, and F1 are in `quantum::ml::metric`. `metric::accuracy` supports
integer-valued class labels and rounds each numeric prediction to the nearest
class, which also preserves the 0.5 threshold behavior for binary probabilities.
Precision, recall, and F1 remain binary metrics and require probability
predictions with 0/1 targets.

## How an evaluation moves through the engine

For a single `forward` call, the backend flow is:

1. `compilePipelineTimeline` checks the input and weight values, invokes the
   feature map, and appends ansatz gates.
2. A simulator executes the combined timeline against the ansatz's circuit
   layout and produces a state vector.
3. The readout computes either the configured Hamiltonian expectation or the
   selected wire's probability.
4. For training, `lossGradient` gets parameter-shift gradients of that output
   and applies the classical loss derivative.
5. `train` averages these gradients over the batch and applies its Adam update.

`getAssembledCircuit` exposes the layout and operation timeline together as a
`CircuitSnapshot`. A UI can inspect the wire layout and gate metadata to draw a
preview; this API does not itself render a diagram.

## Saving and loading models

`save(name)` writes a versioned binary model archive to
`<current working directory>/outputs/<name>.qnn`, creating the `outputs`
directory if necessary. Pass either a base name or a name ending in `.qnn`;
the save name must not include a directory. `load(path)` reads a model from an
explicit path and returns `true` when loading succeeds. Invalid, truncated,
unsupported, or incompatible archives report an exception rather than silently
leaving a partially loaded model.

The built-in model constructor provides a reconstructible architecture:

```cpp
quantum::ml::qnn model(3, 1); // 3 wires; one hardware-efficient ansatz layer
model.setOutputMode(quantum::ml::OutputMode::Probability);
model.setOutputQubit(0);

const auto optimizedWeights =
    model.train(inputs, targets, initialWeights, 50, 0.05);
model.save("classifier");

quantum::ml::qnn restoredModel;
restoredModel.load("outputs/classifier.qnn");
const auto predictions = restoredModel.forwardBatch(inputs);
```

This built-in template is part of the archive definition: it applies an Rx
feature-encoding rotation to each supplied feature wire, followed by the
hardware-efficient ansatz. The archive stores its template identifier, qubit
and layer counts, readout configuration, observables, trained loss, and
weights. This deterministic description reconstructs the same gate topology
without serializing executable code.

For a QNN made from user callbacks, callback code cannot be serialized. Build
the same feature map and ansatz in a compatible model instance before loading
its archive. The loader checks the qubit count and trainable-weight count, then
restores the stored weights, observables, output configuration, and loss. It
does not attempt to compare the behavior or source code of callback functions.
Archives are intended for this engine's current binary format and do not
provide cross-platform endianness conversion or backward compatibility across
future format versions.

`getNumQubits`, `getWeightsCount`, `getWeights`, `getTrainingLoss`, and
`isTrained` expose the restored model state. The
[QNN save/load example](../../example/qml/QNN_Save_Load.cpp) demonstrates
rebuilding a compatible callback-based model, loading its saved parameters,
and displaying the resulting circuit canvas.

## CSV input helper

[`utils/csv_helpers.hpp`](../../engine/include/utils/csv_helpers.hpp) provides
the header-only `quantum::utils::loadCSV(path, hasHeader)` helper for loading a
numeric CSV into `std::vector<std::vector<double>>`. The optional `hasHeader`
argument defaults to `false`; set it to `true` to skip the first non-empty
header row. The loader skips blank lines and reports file, malformed-value,
non-finite-value, and inconsistent-row errors with exceptions.

`splitAndSaveDataset(rows, numerator, denominator, seed, sourcePath, hasHeader)`
shuffles a copy deterministically, returns both partitions, and writes them
beside the source file as `train-<name>.csv` and `test-<name>.csv`. It preserves
the source header when requested and never rewrites the source CSV. Existing
prepared files with those generated names are replaced when the split is
written again.
`prepareTrainData` and `prepareTestData` are also available for writing either
partition independently. Load the generated files with `loadCSV` before
training and evaluation when you want to persist the prepared split.

## Regression data preparation

[`qml/qml_helpers.hpp`](../../engine/include/qml/qml_helpers.hpp) provides
header-only helpers for common numeric regression preparation:
`fitRegressionScaler` learns feature and target ranges from training rows,
`transformRegressionDataset` separates a selected target column and scales
features and targets, and `inverseTransformTargets` converts model outputs back
to the original target scale. The target column and scaling output ranges are
configurable. Fit the scaler only on training rows, then reuse it for test rows
to avoid leaking test-set information into training.

These helpers do not clean data: cleaning rules depend on the dataset and remain
the caller's responsibility. Losses and metrics remain available separately in
`losses.hpp`.

## Student placement example

[`QNN_Placement_Predictor.cpp`](../../example/qml/QNN_Placement_Predictor.cpp)
is a binary-classification template. It encodes normalized CGPA, major status,
and minor status on separate wires, configures wire 0's one-outcome probability
as the placement probability, trains on 0/1 placement labels, and prints the
predictions beside those labels.

The example dataset is deliberately small and illustrative. It demonstrates
how to connect real feature rows and labels to the API; it is not a benchmark
or a claim that the model will generalize. A real application needs
representative training data and a separate validation process.

## Examples

- [`QNN_Classification.cpp`](../../example/qml/QNN_Classification.cpp) shows
  probability readout, binary cross-entropy training, and classification
  metrics.
- [`QNN_Regression.cpp`](../../example/qml/QNN_Regression.cpp) shows
  expectation-value readout and mean-squared-error training.
- [`QNN_Placement_Predictor.cpp`](../../example/qml/QNN_Placement_Predictor.cpp)
  shows a multi-feature placement classification pipeline.

These examples are standalone C++ source files. They are not added as targets
to the application's CMake configuration.
