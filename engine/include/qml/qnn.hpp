#pragma once

#include "ansatz.hpp"
#include "circuit.hpp"
#include "costfxn.hpp"
#include "operators.hpp"

#include <complex>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace quantum::ml {

enum class LossType {
    Automatic,
    MeanSquaredError,
    BinaryCrossEntropy
};

enum class OutputMode {
    Expectation,
    Probability
};

struct CircuitSnapshot {
    quantum::circuit layout;
    quantum::operators operations;
};

class qnn {
public:
    // Callback function type used to transform data features into encoding gates
    using FeatureMap =
        std::function<void(quantum::operators& ops, const std::vector<double>& dataInput)>;

    // Callback function type used to report optimization progress
    using TrainStepCallback =
        std::function<void(std::size_t epoch, double currentLoss, const std::vector<double>& currentWeights)>;

    // Creates an unconfigured model that can load a saved built-in template.
    qnn() = default;

    // Constructor that binds the feature map and variational circuit structure
    qnn(
        std::size_t numQubits,
        FeatureMap encoder,
        const quantum::variational::ansatz& variationalAnsatz);

    // Creates the built-in angle-encoding and hardware-efficient model template
    qnn(std::size_t numQubits, std::size_t layersCount);

    // Evaluates one input using the configured output mode
    double forward(const std::vector<double>& dataInput, const std::vector<double>& weights) const;

    // Evaluates one input using the model's stored trained weights
    double forward(const std::vector<double>& dataInput) const;

    // Evaluates one input against each configured observable
    std::vector<double> forwardValues(
        const std::vector<double>& dataInput,
        const std::vector<double>& weights) const;

    // Evaluates a batch of inputs using the configured output mode
    std::vector<double> forwardBatch(
        const std::vector<std::vector<double>>& batchInputs,
        const std::vector<double>& weights) const;

    // Evaluates a batch using the model's stored trained weights
    std::vector<double> forwardBatch(
        const std::vector<std::vector<double>>& batchInputs) const;

    // Returns the final state vector produced by the circuit pipeline
    std::vector<std::complex<double>> forwardStateVector(
        const std::vector<double>& dataInput,
        const std::vector<double>& weights) const;

    // Returns the probability that an output wire is measured as one
    double forwardProbability(
        const std::vector<double>& dataInput,
        const std::vector<double>& weights,
        std::size_t outputQubit = 0) const;

    // Calculates the configured output gradient using the Parameter-Shift Rule
    std::vector<double> backward(
        const std::vector<double>& dataInput,
        const std::vector<double>& weights,
        double shiftValue = 1.5707963267948966) const;

    // Calculates an output probability gradient using the Parameter-Shift Rule
    std::vector<double> backwardProbability(
        const std::vector<double>& dataInput,
        const std::vector<double>& weights,
        std::size_t outputQubit = 0,
        double shiftValue = 1.5707963267948966) const;

    // Evaluates a scalar training objective for one input and target
    double loss(
        const std::vector<double>& dataInput,
        const std::vector<double>& weights,
        double expectedTarget,
        LossType lossType = LossType::Automatic) const;

    // Evaluates a batch training objective
    double loss(
        const std::vector<std::vector<double>>& dataInputs,
        const std::vector<double>& weights,
        const std::vector<double>& expectedTargets,
        LossType lossType = LossType::Automatic) const;

    // Calculates the gradient of a scalar training objective
    std::vector<double> lossGradient(
        const std::vector<double>& dataInput,
        const std::vector<double>& weights,
        double expectedTarget,
        LossType lossType = LossType::Automatic) const;

    // Trains from caller-provided weights and returns the updated weights
    std::vector<double> train(
        const std::vector<std::vector<double>>& datasetInputs,
        const std::vector<double>& expectedTargets,
        const std::vector<double>& initialWeights,
        std::size_t maxEpochs = 50,
        double learningRate = 0.05,
        LossType lossType = LossType::Automatic);

    // Trains over each row once per epoch using bounded mini-batches
    std::vector<double> train(
        const std::vector<std::vector<double>>& datasetInputs,
        const std::vector<double>& expectedTargets,
        const std::vector<double>& initialWeights,
        std::size_t externalEpochCount,
        std::size_t batchSize,
        double learningRate,
        LossType lossType = LossType::Automatic);

    // Trains while reporting each epoch through a callback
    std::vector<double> train(
        const std::vector<std::vector<double>>& datasetInputs,
        const std::vector<double>& expectedTargets,
        const std::vector<double>& initialWeights,
        TrainStepCallback telemetryHook,
        std::size_t maxEpochs = 50,
        double learningRate = 0.05,
        LossType lossType = LossType::Automatic);

    // Trains in mini-batches while reporting each completed data pass
    std::vector<double> train(
        const std::vector<std::vector<double>>& datasetInputs,
        const std::vector<double>& expectedTargets,
        const std::vector<double>& initialWeights,
        TrainStepCallback telemetryHook,
        std::size_t externalEpochCount,
        std::size_t batchSize,
        double learningRate,
        LossType lossType = LossType::Automatic);

    // Builds a circuit layout and operation timeline for inspection
    CircuitSnapshot getAssembledCircuit(
        const std::vector<double>& sampleInput,
        const std::vector<double>& sampleWeights) const;

    // Builds an inspection snapshot using the model's stored trained weights
    CircuitSnapshot getAssembledCircuit(const std::vector<double>& sampleInput) const;

    // Returns the number of trainable parameters
    std::size_t getWeightsCount() const noexcept;

    // Returns the configured qubit count and the stored training result
    std::size_t getNumQubits() const noexcept;
    std::size_t getLayersCount() const noexcept;
    const std::vector<double>& getWeights() const noexcept;
    double getTrainingLoss() const noexcept;
    bool isTrained() const noexcept;
    OutputMode getOutputMode() const noexcept;
    std::size_t getOutputQubit() const noexcept;

    // Saves and loads a versioned QNN archive
    void save(const std::string& name) const;
    bool load(const std::string& path);

    // Configures the observables used for network outputs
    void setObservables(
        const std::vector<quantum::costfxn::Hamiltonian>& targetObservables);

    // Configures the primary output as an observable expectation or probability
    void setOutputMode(OutputMode outputMode) noexcept;

    // Selects the wire used by probability output mode
    void setOutputQubit(std::size_t outputQubit);

private:
    enum class ArchitectureKind : std::uint8_t {
        Unconfigured,
        UserDefined,
        BuiltInAngleHardwareEfficient
    };

    std::size_t m_qubits = 0;
    FeatureMap m_encoder;
    std::optional<quantum::variational::ansatz> m_ansatzLayer;
    std::vector<quantum::costfxn::Hamiltonian> m_observables;
    OutputMode m_outputMode = OutputMode::Expectation;
    std::size_t m_outputQubit = 0;
    ArchitectureKind m_architectureKind = ArchitectureKind::Unconfigured;
    std::size_t m_layersCount = 0;
    std::vector<double> m_weights;
    double m_trainingLoss = 0.0;
    bool m_isTrained = false;

    // Adds data-encoding and variational operations to one timeline
    void compilePipelineTimeline(
        quantum::operators& timeline,
        const std::vector<double>& dataInput,
        const std::vector<double>& weights) const;
};

} // namespace quantum::ml
