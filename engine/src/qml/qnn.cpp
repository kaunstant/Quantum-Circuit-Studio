#include "qnn.hpp"

#include "gradients.hpp"
#include "losses.hpp"
#include "simulator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace quantum::ml {
namespace {

constexpr std::array<char, 4> kArchiveMagic = {'Q', 'N', 'N', '\0'};
constexpr std::uint32_t kArchiveVersion = 1;
constexpr std::uint64_t kMaximumArchiveItems = 1000000;
constexpr std::uint64_t kMaximumArchiveStringLength = 4096;
constexpr std::uintmax_t kMaximumArchiveSize = 64U * 1024U * 1024U;

template <typename Value>
void writeValue(std::ostream& stream, const Value& value)
{
    static_assert(std::is_trivially_copyable<Value>::value, "Archive values must be trivially copyable");
    stream.write(reinterpret_cast<const char*>(&value), sizeof(Value));
    if (!stream) {
        throw std::runtime_error("Failed to write QNN model archive");
    }
}

template <typename Value>
Value readValue(std::istream& stream)
{
    static_assert(std::is_trivially_copyable<Value>::value, "Archive values must be trivially copyable");
    Value value{};
    stream.read(reinterpret_cast<char*>(&value), sizeof(Value));
    if (!stream) {
        throw std::runtime_error("QNN model archive is truncated");
    }
    return value;
}

void writeString(std::ostream& stream, const std::string& value)
{
    const std::uint64_t length = static_cast<std::uint64_t>(value.size());
    writeValue(stream, length);
    stream.write(value.data(), static_cast<std::streamsize>(value.size()));
    if (!stream) {
        throw std::runtime_error("Failed to write QNN model archive string");
    }
}

std::string readString(std::istream& stream)
{
    const std::uint64_t length = readValue<std::uint64_t>(stream);
    if (length > kMaximumArchiveStringLength) {
        throw std::runtime_error("QNN model archive string exceeds the supported limit");
    }
    std::string value(static_cast<std::size_t>(length), '\0');
    stream.read(value.data(), static_cast<std::streamsize>(length));
    if (!stream) {
        throw std::runtime_error("QNN model archive contains a truncated string");
    }
    return value;
}

void writeObservables(
    std::ostream& stream,
    const std::vector<quantum::costfxn::Hamiltonian>& observables)
{
    writeValue(stream, static_cast<std::uint64_t>(observables.size()));
    for (const auto& observable : observables) {
        writeValue(stream, static_cast<std::uint64_t>(observable.terms().size()));
        for (const auto& term : observable.terms()) {
            writeValue(stream, term.coefficient);
            writeString(stream, term.operators);
        }
    }
}

std::vector<quantum::costfxn::Hamiltonian> readObservables(
    std::istream& stream,
    std::size_t numQubits)
{
    const std::uint64_t observableCount = readValue<std::uint64_t>(stream);
    if (observableCount == 0 || observableCount > kMaximumArchiveItems) {
        throw std::runtime_error("QNN model archive has an invalid observable count");
    }

    std::vector<quantum::costfxn::Hamiltonian> observables;
    observables.reserve(static_cast<std::size_t>(observableCount));
    for (std::uint64_t observableIndex = 0; observableIndex < observableCount; ++observableIndex) {
        const std::uint64_t termCount = readValue<std::uint64_t>(stream);
        if (termCount == 0 || termCount > kMaximumArchiveItems) {
            throw std::runtime_error("QNN model archive has an invalid Hamiltonian term count");
        }
        quantum::costfxn::Hamiltonian observable;
        for (std::uint64_t termIndex = 0; termIndex < termCount; ++termIndex) {
            const double coefficient = readValue<double>(stream);
            const std::string pauliString = readString(stream);
            if (!std::isfinite(coefficient) || pauliString.size() != numQubits) {
                throw std::runtime_error("QNN model archive contains an invalid Hamiltonian term");
            }
            observable.append(coefficient, pauliString);
        }
        observables.push_back(std::move(observable));
    }
    return observables;
}

void configureDefaultObservable(
    std::size_t numQubits,
    std::vector<quantum::costfxn::Hamiltonian>& observables)
{
    std::string defaultPauli(numQubits, 'I');
    defaultPauli[0] = 'Z';
    quantum::costfxn::Hamiltonian defaultObservable;
    defaultObservable.append(1.0, defaultPauli);
    observables.push_back(std::move(defaultObservable));
}

void validateFiniteValues(const std::vector<double>& values, const char* description)
{
    for (const double value : values) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument(std::string(description) + " must be finite");
        }
    }
}

void validateTrainingData(
    const std::vector<std::vector<double>>& inputs,
    const std::vector<double>& targets,
    const std::vector<double>& weights,
    std::size_t expectedWeightCount)
{
    if (inputs.empty()) {
        throw std::invalid_argument("Training data cannot be empty");
    }
    if (inputs.size() != targets.size()) {
        throw std::invalid_argument("Training input and target counts must match");
    }
    if (weights.size() != expectedWeightCount) {
        throw std::invalid_argument("Initial weight count does not match the QNN");
    }
    validateFiniteValues(weights, "Initial weights");
    validateFiniteValues(targets, "Training targets");
    for (const auto& input : inputs) {
        validateFiniteValues(input, "Training input values");
    }
}

double batchLoss(
    const qnn& model,
    const std::vector<std::vector<double>>& inputs,
    const std::vector<double>& targets,
    const std::vector<double>& weights,
    LossType lossType)
{
    double total = 0.0;
    for (std::size_t sample = 0; sample < inputs.size(); ++sample) {
        total += model.loss(inputs[sample], weights, targets[sample], lossType);
    }
    const double value = total / static_cast<double>(inputs.size());
    if (!std::isfinite(value)) {
        throw std::runtime_error("QNN batch loss is non-finite");
    }
    return value;
}

} // namespace

// Constructor that binds the feature map and variational circuit structure
qnn::qnn(
    std::size_t numQubits,
    FeatureMap encoder,
    const quantum::variational::ansatz& variationalAnsatz)
    : m_qubits(numQubits),
      m_encoder(std::move(encoder)),
      m_ansatzLayer(variationalAnsatz),
      m_architectureKind(ArchitectureKind::UserDefined)
{
    if (m_qubits == 0) {
        throw std::invalid_argument("A QNN requires at least one qubit");
    }
    if (!m_encoder) {
        throw std::invalid_argument("A QNN requires a feature-map callback");
    }
    if (m_ansatzLayer->numQubits() != m_qubits) {
        throw std::invalid_argument(
            "QNN and variational circuit qubit counts must match");
    }

    configureDefaultObservable(m_qubits, m_observables);
}

// Creates the built-in angle-encoding and hardware-efficient model template
qnn::qnn(std::size_t numQubits, std::size_t layersCount)
    : m_qubits(numQubits),
      m_encoder([numQubits](quantum::operators& operations, const std::vector<double>& features) {
          if (features.size() > numQubits) {
              throw std::invalid_argument("Built-in QNN input has more features than circuit qubits");
          }
          for (std::size_t wire = 0; wire < features.size(); ++wire) {
              operations.rx(static_cast<int>(wire), features[wire]);
          }
      }),
      m_ansatzLayer(std::in_place, numQubits, layersCount),
      m_architectureKind(ArchitectureKind::BuiltInAngleHardwareEfficient),
      m_layersCount(layersCount)
{
    if (m_qubits == 0) {
        throw std::invalid_argument("A QNN requires at least one qubit");
    }
    configureDefaultObservable(m_qubits, m_observables);
}

// Adds data-encoding and variational operations to one timeline
void qnn::compilePipelineTimeline(
    quantum::operators& timeline,
    const std::vector<double>& dataInput,
    const std::vector<double>& weights) const
{
    if (!m_ansatzLayer || !m_encoder) {
        throw std::logic_error("QNN architecture has not been configured");
    }
    if (weights.size() != getWeightsCount()) {
        throw std::invalid_argument("Weight count does not match the QNN parameter count");
    }
    validateFiniteValues(dataInput, "QNN input values");
    validateFiniteValues(weights, "QNN weights");

    m_encoder(timeline, dataInput);
    m_ansatzLayer->generateOperators(timeline, weights);
}

// Returns the final state vector produced by the circuit pipeline
std::vector<std::complex<double>> qnn::forwardStateVector(
    const std::vector<double>& dataInput,
    const std::vector<double>& weights) const
{
    quantum::operators timeline;
    compilePipelineTimeline(timeline, dataInput, weights);
    quantum::simulator simulation(*m_ansatzLayer, timeline, false);
    simulation.run();
    return simulation.getStateVector();
}

// Evaluates one input against the primary observable
double qnn::forward(
    const std::vector<double>& dataInput,
    const std::vector<double>& weights) const
{
    if (m_outputMode == OutputMode::Probability) {
        return forwardProbability(dataInput, weights, m_outputQubit);
    }
    const auto state = forwardStateVector(dataInput, weights);
    return quantum::costfxn::expectation(m_observables.front(), state);
}

// Evaluates one input against each configured observable
std::vector<double> qnn::forwardValues(
    const std::vector<double>& dataInput,
    const std::vector<double>& weights) const
{
    const auto state = forwardStateVector(dataInput, weights);
    std::vector<double> values;
    values.reserve(m_observables.size());
    for (const auto& observable : m_observables) {
        values.push_back(quantum::costfxn::expectation(observable, state));
    }
    return values;
}

// Evaluates a batch of inputs against the primary observable
std::vector<double> qnn::forwardBatch(
    const std::vector<std::vector<double>>& batchInputs,
    const std::vector<double>& weights) const
{
    std::vector<double> values;
    values.reserve(batchInputs.size());
    for (const auto& input : batchInputs) {
        values.push_back(forward(input, weights));
    }
    return values;
}

// Evaluates one input using the model's stored trained weights
double qnn::forward(const std::vector<double>& dataInput) const
{
    if (!m_isTrained) {
        throw std::logic_error("QNN has no stored trained weights");
    }
    return forward(dataInput, m_weights);
}

// Evaluates a batch using the model's stored trained weights
std::vector<double> qnn::forwardBatch(
    const std::vector<std::vector<double>>& batchInputs) const
{
    if (!m_isTrained) {
        throw std::logic_error("QNN has no stored trained weights");
    }
    return forwardBatch(batchInputs, m_weights);
}

// Returns the probability that an output wire is measured as one
double qnn::forwardProbability(
    const std::vector<double>& dataInput,
    const std::vector<double>& weights,
    std::size_t outputQubit) const
{
    if (outputQubit >= m_qubits) {
        throw std::out_of_range("QNN output qubit is outside the circuit");
    }
    const auto state = forwardStateVector(dataInput, weights);
    const std::size_t mask = std::size_t{1} << outputQubit;
    double probability = 0.0;
    for (std::size_t basis = 0; basis < state.size(); ++basis) {
        if ((basis & mask) != 0) {
            probability += std::norm(state[basis]);
        }
    }
    if (!std::isfinite(probability)) {
        throw std::runtime_error("QNN output probability is non-finite");
    }
    constexpr double probabilityTolerance = 1e-12;
    if (probability < -probabilityTolerance ||
        probability > 1.0 + probabilityTolerance) {
        throw std::runtime_error("QNN output probability is outside [0, 1]");
    }
    return std::clamp(probability, 0.0, 1.0);
}

// Calculates the observable gradient using the Parameter-Shift Rule
std::vector<double> qnn::backward(
    const std::vector<double>& dataInput,
    const std::vector<double>& weights,
    double shiftValue) const
{
    const auto observableFunction =
        [this, &dataInput](const std::vector<double>& candidateWeights) {
            return forward(dataInput, candidateWeights);
        };
    return parameterShiftGradient(observableFunction, weights, shiftValue);
}

// Calculates an output probability gradient using the Parameter-Shift Rule
std::vector<double> qnn::backwardProbability(
    const std::vector<double>& dataInput,
    const std::vector<double>& weights,
    std::size_t outputQubit,
    double shiftValue) const
{
    const auto probabilityFunction =
        [this, &dataInput, outputQubit](const std::vector<double>& candidateWeights) {
            return forwardProbability(dataInput, candidateWeights, outputQubit);
        };
    return parameterShiftGradient(probabilityFunction, weights, shiftValue);
}

// Evaluates a scalar training objective for one input and target
double qnn::loss(
    const std::vector<double>& dataInput,
    const std::vector<double>& weights,
    double expectedTarget,
    LossType lossType) const
{
    if (!std::isfinite(expectedTarget)) {
        throw std::invalid_argument("Training target must be finite");
    }

    if (lossType == LossType::Automatic) {
        lossType = m_outputMode == OutputMode::Probability
            ? LossType::BinaryCrossEntropy
            : LossType::MeanSquaredError;
    }
    if (lossType == LossType::BinaryCrossEntropy) {
        return quantum::ml::loss::binaryCrossEntropy(
            {forwardProbability(dataInput, weights, m_outputQubit)},
            {expectedTarget});
    }
    if (lossType == LossType::MeanSquaredError) {
        return quantum::ml::loss::meanSquaredError(
            {forward(dataInput, weights)},
            {expectedTarget});
    }
    throw std::invalid_argument("Unsupported QNN loss type");
}

// Evaluates a batch training objective
double qnn::loss(
    const std::vector<std::vector<double>>& dataInputs,
    const std::vector<double>& weights,
    const std::vector<double>& expectedTargets,
    LossType lossType) const
{
    if (dataInputs.empty()) {
        throw std::invalid_argument("Loss dataset cannot be empty");
    }
    if (dataInputs.size() != expectedTargets.size()) {
        throw std::invalid_argument("Training input and target counts must match");
    }
    double total = 0.0;
    for (std::size_t sample = 0; sample < dataInputs.size(); ++sample) {
        total += loss(dataInputs[sample], weights, expectedTargets[sample], lossType);
    }
    const double average = total / static_cast<double>(dataInputs.size());
    if (!std::isfinite(average)) {
        throw std::runtime_error("QNN batch loss is non-finite");
    }
    return average;
}

// Calculates the gradient of a scalar training objective
std::vector<double> qnn::lossGradient(
    const std::vector<double>& dataInput,
    const std::vector<double>& weights,
    double expectedTarget,
    LossType lossType) const
{
    if (!std::isfinite(expectedTarget)) {
        throw std::invalid_argument("Training target must be finite");
    }

    if (lossType == LossType::Automatic) {
        lossType = m_outputMode == OutputMode::Probability
            ? LossType::BinaryCrossEntropy
            : LossType::MeanSquaredError;
    }
    if (lossType == LossType::MeanSquaredError) {
        const double difference = forward(dataInput, weights) - expectedTarget;
        std::vector<double> gradient = backward(dataInput, weights);
        for (double& component : gradient) {
            component *= 2.0 * difference;
            if (!std::isfinite(component)) {
                throw std::runtime_error("QNN loss gradient is non-finite");
            }
        }
        return gradient;
    }
    if (lossType == LossType::BinaryCrossEntropy) {
        if (expectedTarget < 0.0 || expectedTarget > 1.0) {
            throw std::invalid_argument(
                "Binary cross-entropy target must be in [0, 1]");
        }
        constexpr double epsilon = 1e-12;
        const double probability = forwardProbability(dataInput, weights, m_outputQubit);
        std::vector<double> gradient =
            backwardProbability(dataInput, weights, m_outputQubit);
        if (probability <= epsilon || probability >= 1.0 - epsilon) {
            std::fill(gradient.begin(), gradient.end(), 0.0);
            return gradient;
        }
        const double derivative =
            (probability - expectedTarget) / (probability * (1.0 - probability));
        for (double& component : gradient) {
            component *= derivative;
            if (!std::isfinite(component)) {
                throw std::runtime_error("QNN loss gradient is non-finite");
            }
        }
        return gradient;
    }
    throw std::invalid_argument("Unsupported QNN loss type");
}

// Trains from caller-provided weights using full-dataset batches
std::vector<double> qnn::train(
    const std::vector<std::vector<double>>& datasetInputs,
    const std::vector<double>& expectedTargets,
    const std::vector<double>& initialWeights,
    std::size_t maxEpochs,
    double learningRate,
    LossType lossType)
{
    return train(
        datasetInputs,
        expectedTargets,
        initialWeights,
        TrainStepCallback{},
        maxEpochs,
        datasetInputs.size(),
        learningRate,
        lossType);
}

// Trains from caller-provided weights using bounded mini-batches
std::vector<double> qnn::train(
    const std::vector<std::vector<double>>& datasetInputs,
    const std::vector<double>& expectedTargets,
    const std::vector<double>& initialWeights,
    std::size_t externalEpochCount,
    std::size_t batchSize,
    double learningRate,
    LossType lossType)
{
    return train(
        datasetInputs,
        expectedTargets,
        initialWeights,
        TrainStepCallback{},
        externalEpochCount,
        batchSize,
        learningRate,
        lossType);
}

// Trains the full dataset and reports each completed pass through a callback
std::vector<double> qnn::train(
    const std::vector<std::vector<double>>& datasetInputs,
    const std::vector<double>& expectedTargets,
    const std::vector<double>& initialWeights,
    TrainStepCallback telemetryHook,
    std::size_t maxEpochs,
    double learningRate,
    LossType lossType)
{
    return train(
        datasetInputs,
        expectedTargets,
        initialWeights,
        std::move(telemetryHook),
        maxEpochs,
        datasetInputs.size(),
        learningRate,
        lossType);
}

// Trains in mini-batches and reports each completed data pass
std::vector<double> qnn::train(
    const std::vector<std::vector<double>>& datasetInputs,
    const std::vector<double>& expectedTargets,
    const std::vector<double>& initialWeights,
    TrainStepCallback telemetryHook,
    std::size_t externalEpochCount,
    std::size_t batchSize,
    double learningRate,
    LossType lossType)
{
    validateTrainingData(
        datasetInputs,
        expectedTargets,
        initialWeights,
        getWeightsCount());
    if (!std::isfinite(learningRate) || learningRate <= 0.0) {
        throw std::invalid_argument("Training learning rate must be positive and finite");
    }
    if (batchSize == 0) {
        throw std::invalid_argument("Training batch size must be positive");
    }
    if (lossType == LossType::Automatic) {
        lossType = m_outputMode == OutputMode::Probability
            ? LossType::BinaryCrossEntropy
            : LossType::MeanSquaredError;
    }
    if (lossType != LossType::MeanSquaredError &&
        lossType != LossType::BinaryCrossEntropy) {
        throw std::invalid_argument("Unsupported QNN loss type");
    }

    std::vector<double> weights = initialWeights;
    std::vector<double> firstMoment(weights.size(), 0.0);
    std::vector<double> secondMoment(weights.size(), 0.0);
    constexpr double betaOne = 0.9;
    constexpr double betaTwo = 0.999;
    constexpr double epsilon = 1e-8;
    std::size_t updateCount = 0;

    for (std::size_t epochIndex = 0;
         epochIndex < externalEpochCount;
         ++epochIndex) {
        const std::size_t epoch = epochIndex + 1;
        for (std::size_t batchStart = 0;
             batchStart < datasetInputs.size();) {
            const std::size_t batchEnd =
                batchStart + std::min(batchSize, datasetInputs.size() - batchStart);
            const std::size_t samplesInBatch = batchEnd - batchStart;
            std::vector<double> gradient(weights.size(), 0.0);
            for (std::size_t sample = batchStart; sample < batchEnd; ++sample) {
                const std::vector<double> sampleGradient =
                    lossGradient(
                        datasetInputs[sample],
                        weights,
                        expectedTargets[sample],
                        lossType);
                for (std::size_t parameter = 0; parameter < weights.size(); ++parameter) {
                    gradient[parameter] += sampleGradient[parameter];
                }
            }

            const double sampleCount = static_cast<double>(samplesInBatch);
            ++updateCount;
            const double firstBiasCorrection =
                1.0 - std::pow(betaOne, static_cast<double>(updateCount));
            const double secondBiasCorrection =
                1.0 - std::pow(betaTwo, static_cast<double>(updateCount));
            for (std::size_t parameter = 0; parameter < weights.size(); ++parameter) {
                gradient[parameter] /= sampleCount;
                if (!std::isfinite(gradient[parameter])) {
                    throw std::runtime_error("QNN training gradient is non-finite");
                }
                firstMoment[parameter] =
                    betaOne * firstMoment[parameter] +
                    (1.0 - betaOne) * gradient[parameter];
                secondMoment[parameter] =
                    betaTwo * secondMoment[parameter] +
                    (1.0 - betaTwo) * gradient[parameter] * gradient[parameter];
                const double correctedFirst = firstMoment[parameter] / firstBiasCorrection;
                const double correctedSecond = secondMoment[parameter] / secondBiasCorrection;
                weights[parameter] -= learningRate * correctedFirst /
                    (std::sqrt(correctedSecond) + epsilon);
                if (!std::isfinite(weights[parameter])) {
                    throw std::runtime_error("QNN training produced a non-finite weight");
                }
            }
            batchStart = batchEnd;
        }

        if (telemetryHook) {
            telemetryHook(
                epoch,
                batchLoss(*this, datasetInputs, expectedTargets, weights, lossType),
                weights);
        }
    }
    m_weights = weights;
    m_trainingLoss = batchLoss(*this, datasetInputs, expectedTargets, weights, lossType);
    m_isTrained = true;
    return weights;
}
// Builds a circuit layout and operation timeline for inspection
CircuitSnapshot qnn::getAssembledCircuit(
    const std::vector<double>& sampleInput,
    const std::vector<double>& sampleWeights) const
{
    CircuitSnapshot snapshot{quantum::circuit(m_qubits), quantum::operators{}};
    compilePipelineTimeline(snapshot.operations, sampleInput, sampleWeights);
    return snapshot;
}

// Builds an inspection snapshot using the model's stored trained weights
CircuitSnapshot qnn::getAssembledCircuit(const std::vector<double>& sampleInput) const
{
    if (!m_isTrained) {
        throw std::logic_error("QNN has no stored trained weights");
    }
    return getAssembledCircuit(sampleInput, m_weights);
}

// Returns the number of trainable parameters
std::size_t qnn::getWeightsCount() const noexcept
{
    return m_ansatzLayer ? m_ansatzLayer->getParamCount() : 0;
}

// Returns the configured qubit count
std::size_t qnn::getNumQubits() const noexcept
{
    return m_qubits;
}

// Returns the built-in ansatz layer count, or zero for user-defined architectures
std::size_t qnn::getLayersCount() const noexcept
{
    return m_layersCount;
}

// Returns the model's stored weights
const std::vector<double>& qnn::getWeights() const noexcept
{
    return m_weights;
}

// Returns the final training loss
double qnn::getTrainingLoss() const noexcept
{
    return m_trainingLoss;
}

// Returns whether this model contains trained weights
bool qnn::isTrained() const noexcept
{
    return m_isTrained;
}

// Returns the configured output mode
OutputMode qnn::getOutputMode() const noexcept
{
    return m_outputMode;
}

// Returns the configured probability output wire
std::size_t qnn::getOutputQubit() const noexcept
{
    return m_outputQubit;
}

// Saves a versioned QNN archive in the outputs directory
void qnn::save(const std::string& name) const
{
    if (!m_isTrained || !m_ansatzLayer || !m_encoder) {
        throw std::logic_error("Only a trained, configured QNN can be saved");
    }
    if (name.empty()) {
        throw std::invalid_argument("QNN archive name cannot be empty");
    }
    validateFiniteValues(m_weights, "Stored QNN weights");
    if (!std::isfinite(m_trainingLoss)) {
        throw std::runtime_error("Stored QNN training loss is non-finite");
    }

    const std::filesystem::path requestedName(name);
    if (requestedName.is_absolute() || requestedName.has_parent_path() ||
        requestedName.filename() != requestedName) {
        throw std::invalid_argument("QNN archive name must not contain a directory path");
    }
    std::filesystem::path filename = requestedName;
    if (filename.extension() != ".qnn") {
        filename += ".qnn";
    }
    const std::filesystem::path outputDirectory =
        std::filesystem::current_path() / "outputs";
    std::filesystem::create_directories(outputDirectory);
    const std::filesystem::path archivePath = outputDirectory / filename;
    std::ofstream archive(archivePath, std::ios::binary | std::ios::trunc);
    if (!archive) {
        throw std::runtime_error("Unable to create QNN model archive: " + archivePath.string());
    }

    archive.write(kArchiveMagic.data(), static_cast<std::streamsize>(kArchiveMagic.size()));
    writeValue(archive, kArchiveVersion);
    writeValue(archive, static_cast<std::uint8_t>(m_architectureKind));
    writeValue(archive, static_cast<std::uint64_t>(m_qubits));
    writeValue(archive, static_cast<std::uint64_t>(m_layersCount));
    writeValue(archive, static_cast<std::uint64_t>(m_weights.size()));
    writeValue(archive, static_cast<std::uint8_t>(m_outputMode));
    writeValue(archive, static_cast<std::uint64_t>(m_outputQubit));
    writeValue(archive, static_cast<std::uint8_t>(m_isTrained ? 1 : 0));
    writeValue(archive, m_trainingLoss);
    writeObservables(archive, m_observables);
    for (const double weight : m_weights) {
        writeValue(archive, weight);
    }
    archive.flush();
    if (!archive) {
        throw std::runtime_error("Failed to finish writing QNN model archive: " + archivePath.string());
    }
    archive.close();
    if (!archive) {
        throw std::runtime_error("Failed to close QNN model archive: " + archivePath.string());
    }
}

// Loads a QNN archive into a compatible model or reconstructs its built-in template
bool qnn::load(const std::string& path)
{
    const std::filesystem::path archivePath(path);
    std::error_code filesystemError;
    const std::uintmax_t archiveSize = std::filesystem::file_size(archivePath, filesystemError);
    if (filesystemError) {
        throw std::runtime_error(
            "Unable to read QNN model archive " + archivePath.string() + ": " +
            filesystemError.message());
    }
    if (archiveSize > kMaximumArchiveSize) {
        throw std::runtime_error("QNN model archive exceeds the supported size limit");
    }
    std::ifstream archive(archivePath, std::ios::binary);
    if (!archive) {
        throw std::runtime_error("Unable to open QNN model archive: " + archivePath.string());
    }

    std::array<char, kArchiveMagic.size()> magic{};
    archive.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (!archive || magic != kArchiveMagic) {
        throw std::runtime_error("Invalid QNN model archive signature");
    }
    if (readValue<std::uint32_t>(archive) != kArchiveVersion) {
        throw std::runtime_error("Unsupported QNN model archive version");
    }
    const auto archiveKind = static_cast<ArchitectureKind>(readValue<std::uint8_t>(archive));
    const std::uint64_t qubitCountValue = readValue<std::uint64_t>(archive);
    const std::uint64_t layerCountValue = readValue<std::uint64_t>(archive);
    const std::uint64_t weightCountValue = readValue<std::uint64_t>(archive);
    const std::uint8_t outputModeValue = readValue<std::uint8_t>(archive);
    const std::uint64_t outputQubitValue = readValue<std::uint64_t>(archive);
    const std::uint8_t trainedFlag = readValue<std::uint8_t>(archive);
    const double trainingLoss = readValue<double>(archive);

    if ((archiveKind != ArchitectureKind::UserDefined &&
         archiveKind != ArchitectureKind::BuiltInAngleHardwareEfficient) ||
        qubitCountValue == 0 || qubitCountValue > kMaximumArchiveItems ||
        layerCountValue > kMaximumArchiveItems || weightCountValue > kMaximumArchiveItems ||
        (outputModeValue != static_cast<std::uint8_t>(OutputMode::Expectation) &&
         outputModeValue != static_cast<std::uint8_t>(OutputMode::Probability)) ||
        outputQubitValue >= qubitCountValue || trainedFlag != 1 ||
        !std::isfinite(trainingLoss)) {
        throw std::runtime_error("QNN model archive contains invalid model metadata");
    }
    const std::size_t numQubits = static_cast<std::size_t>(qubitCountValue);
    const std::size_t layersCount = static_cast<std::size_t>(layerCountValue);
    const std::size_t weightCount = static_cast<std::size_t>(weightCountValue);
    if (archiveKind == ArchitectureKind::BuiltInAngleHardwareEfficient &&
        (layersCount == 0 || weightCount != 2 * numQubits * layersCount)) {
        throw std::runtime_error("QNN built-in archive has inconsistent layer or weight counts");
    }
    if (archiveKind == ArchitectureKind::UserDefined && layerCountValue != 0) {
        throw std::runtime_error("QNN user-defined archive contains built-in layer metadata");
    }

    auto observables = readObservables(archive, numQubits);
    std::vector<double> weights(weightCount);
    for (double& weight : weights) {
        weight = readValue<double>(archive);
        if (!std::isfinite(weight)) {
            throw std::runtime_error("QNN model archive contains a non-finite weight");
        }
    }
    if (archive.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("QNN model archive contains unexpected trailing data");
    }

    const OutputMode outputMode = static_cast<OutputMode>(outputModeValue);
    const std::size_t outputQubit = static_cast<std::size_t>(outputQubitValue);
    if (archiveKind == ArchitectureKind::BuiltInAngleHardwareEfficient) {
        if (m_architectureKind == ArchitectureKind::UserDefined) {
            throw std::invalid_argument(
                "Cannot replace a user-defined QNN architecture while loading a built-in archive");
        }
        if (m_architectureKind == ArchitectureKind::BuiltInAngleHardwareEfficient &&
            (m_qubits != numQubits || m_layersCount != layersCount)) {
            throw std::invalid_argument(
                "Built-in QNN archive topology does not match the configured model");
        }
        if (m_architectureKind == ArchitectureKind::Unconfigured) {
            m_qubits = numQubits;
            m_encoder = [numQubits](
                            quantum::operators& operations,
                            const std::vector<double>& features) {
                if (features.size() > numQubits) {
                    throw std::invalid_argument(
                        "Built-in QNN input has more features than circuit qubits");
                }
                for (std::size_t wire = 0; wire < features.size(); ++wire) {
                    operations.rx(static_cast<int>(wire), features[wire]);
                }
            };
            m_ansatzLayer.emplace(numQubits, layersCount);
            m_architectureKind = ArchitectureKind::BuiltInAngleHardwareEfficient;
            m_layersCount = layersCount;
        }
    } else if (m_architectureKind != ArchitectureKind::UserDefined ||
               !m_ansatzLayer || m_qubits != numQubits ||
               m_ansatzLayer->getParamCount() != weightCount) {
        throw std::invalid_argument(
            "A user-defined QNN archive requires a compatible preconstructed architecture");
    }

    m_weights = std::move(weights);
    m_observables = std::move(observables);
    m_outputMode = outputMode;
    m_outputQubit = outputQubit;
    m_trainingLoss = trainingLoss;
    m_isTrained = true;
    return true;
}

// Configures the observables used for network outputs
void qnn::setObservables(
    const std::vector<quantum::costfxn::Hamiltonian>& targetObservables)
{
    if (targetObservables.empty()) {
        throw std::invalid_argument("A QNN requires at least one output observable");
    }
    for (const auto& observable : targetObservables) {
        if (observable.terms().empty() || observable.numQubits() != m_qubits) {
            throw std::invalid_argument(
                "Each QNN observable must be non-empty and match the circuit qubit count");
        }
    }
    m_observables = targetObservables;
}

// Configures the primary output as an observable expectation or probability
void qnn::setOutputMode(OutputMode outputMode) noexcept
{
    m_outputMode = outputMode;
}

// Selects the wire used by probability output mode
void qnn::setOutputQubit(std::size_t outputQubit)
{
    if (outputQubit >= m_qubits) {
        throw std::out_of_range("QNN output qubit is outside the circuit");
    }
    m_outputQubit = outputQubit;
}

} // namespace quantum::ml
