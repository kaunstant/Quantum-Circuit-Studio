#include "QNN.hpp"

#include "Gradients.hpp"
#include "Losses.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace QNN
{
QNN::QNN()
    : outputMode_(OutputMode::Expectation),
      outputQubit_(0),
      hasObservable_(false)
{
}

void QNN::setFeatureMap(FeatureMap featureMap)
{
    featureMap_ = std::move(featureMap);
}

void QNN::setAnsatz(Ansatz ansatz)
{
    ansatz_ = std::move(ansatz);
}

void QNN::setParameters(std::vector<double> parameters)
{
    parameters_ = std::move(parameters);
}

void QNN::setObservable(Variational::Hamiltonian observable)
{
    observable_ = std::move(observable);
    observables_ = {observable_};
    hasObservable_ = true;
}

void QNN::setObservables(std::vector<Variational::Hamiltonian> observables)
{
    if (observables.empty())
    {
        throw std::invalid_argument("QNN requires at least one observable.");
    }

    observable_ = observables.front();
    observables_ = std::move(observables);
    hasObservable_ = true;
}

void QNN::setOutputMode(OutputMode mode)
{
    outputMode_ = mode;
}

void QNN::setOutputQubit(int qubit)
{
    if (qubit < 0)
    {
        throw std::invalid_argument("QNN output qubit cannot be negative.");
    }

    outputQubit_ = qubit;
    outputQubits_ = {qubit};
}

void QNN::setOutputQubits(std::vector<int> qubits)
{
    if (qubits.empty())
    {
        throw std::invalid_argument("QNN requires at least one output qubit.");
    }

    for (int qubit : qubits)
    {
        if (qubit < 0)
        {
            throw std::invalid_argument("QNN output qubits cannot be negative.");
        }
    }

    outputQubit_ = qubits.front();
    outputQubits_ = std::move(qubits);
}

void QNN::validateReady() const
{
    if (!featureMap_ || !ansatz_)
    {
        throw std::logic_error("QNN requires both a feature map and an ansatz.");
    }

    if (outputMode_ == OutputMode::Expectation && !hasObservable_)
    {
        throw std::logic_error(
            "Expectation output requires an observable Hamiltonian.");
    }

    if (outputMode_ == OutputMode::Probability && outputQubits_.empty())
    {
        throw std::logic_error("Probability output requires output qubits.");
    }
}

QuantumCircuit QNN::buildCircuit(
    const std::vector<double>& features,
    const std::vector<double>& parameters) const
{
    validateReady();

    QuantumCircuit featureCircuit = featureMap_(features);
    QuantumCircuit ansatzCircuit = ansatz_(parameters);
    featureCircuit.append(ansatzCircuit);
    return featureCircuit;
}

ForwardResult QNN::forwardPass(
    const std::vector<double>& features) const
{
    QuantumSimulator simulator;
    const SimulationResult result =
        simulator.run(buildCircuit(features, parameters_));

    ForwardResult output;
    output.state = result.state;
    output.probabilities = QuantumSimulator::probabilities(result.state);

    if (outputMode_ == OutputMode::Expectation)
    {
        output.value = observables_.front().expectation(result.state);
    }
    else
    {
        if (outputQubits_.empty())
        {
            throw std::logic_error("Probability output requires output qubits.");
        }

        if (outputQubit_ >= static_cast<int>(std::log2(result.state.size())))
        {
            throw std::out_of_range("QNN output qubit exceeds circuit width.");
        }

        for (std::size_t basis = 0; basis < output.probabilities.size(); ++basis)
        {
            if ((basis & (std::size_t{1} << outputQubit_)) != 0)
            {
                output.value += output.probabilities[basis];
            }
        }
    }

    return output;
}

double QNN::forward(const std::vector<double>& features) const
{
    return forwardPass(features).value;
}

std::vector<double> QNN::forwardBatch(
    const std::vector<std::vector<double>>& batch) const
{
    std::vector<double> outputs;
    outputs.reserve(batch.size());

    for (const auto& features : batch)
    {
        outputs.push_back(forward(features));
    }

    return outputs;
}

std::vector<double> QNN::backward(
    const std::vector<double>& features,
    double shift) const
{
    validateReady();
    return parameterShift(
        [this, &features](const std::vector<double>& parameters)
        {
            QuantumSimulator simulator;
            const SimulationResult result =
                simulator.run(buildCircuit(features, parameters));

            if (outputMode_ == OutputMode::Expectation)
            {
                return observable_.expectation(result.state);
            }

            const std::vector<double> probabilities =
                QuantumSimulator::probabilities(result.state);
            double value = 0.0;
            for (std::size_t basis = 0; basis < probabilities.size(); ++basis)
            {
                if ((basis & (std::size_t{1} << outputQubit_)) != 0)
                {
                    value += probabilities[basis];
                }
            }
            return value;
        },
        parameters_,
        shift);
}

double QNN::loss(
    const std::vector<double>& features,
    double target) const
{
    if (outputMode_ == OutputMode::Probability)
    {
        return binaryCrossEntropy(forward(features), target);
    }

    const double difference = forward(features) - target;
    return difference * difference;
}

std::vector<double> QNN::lossGradient(
    const std::vector<double>& features,
    double target) const
{
    const double prediction = forward(features);
    const double scale = outputMode_ == OutputMode::Probability
        ? (prediction - target) /
          std::max(prediction * (1.0 - prediction), 1e-12)
        : 2.0 * (prediction - target);

    std::vector<double> gradients = backward(features);
    for (double& gradient : gradients)
    {
        gradient *= scale;
    }
    return gradients;
}

Variational::OptimizationResult QNN::train(
    const std::vector<std::vector<double>>& features,
    const std::vector<double>& targets,
    int iterations,
    double learningRate)
{
    if (features.empty() || features.size() != targets.size() ||
        iterations <= 0 || learningRate <= 0.0)
    {
        throw std::invalid_argument("Invalid QNN training data or settings.");
    }

    Variational::OptimizationResult result;
    result.parameters = parameters_;

    for (int iteration = 0; iteration < iterations; ++iteration)
    {
        std::vector<double> gradient(parameters_.size(), 0.0);
        double totalLoss = 0.0;

        for (std::size_t sample = 0; sample < features.size(); ++sample)
        {
            totalLoss += loss(features[sample], targets[sample]);
            const std::vector<double> sampleGradient =
                lossGradient(features[sample], targets[sample]);

            for (std::size_t index = 0; index < gradient.size(); ++index)
            {
                gradient[index] += sampleGradient[index];
            }
        }

        for (std::size_t index = 0; index < parameters_.size(); ++index)
        {
            parameters_[index] -= learningRate *
                gradient[index] / static_cast<double>(features.size());
        }

        result.iterations = iteration + 1;
        result.value = totalLoss / static_cast<double>(features.size());
        result.parameters = parameters_;
    }

    return result;
}

std::vector<double> QNN::forwardValues(
    const std::vector<double>& features) const
{
    validateReady();
    QuantumSimulator simulator;
    const SimulationResult result =
        simulator.run(buildCircuit(features, parameters_));
    const std::vector<double> probabilities =
        QuantumSimulator::probabilities(result.state);

    if (outputMode_ == OutputMode::Expectation)
    {
        std::vector<double> values;
        for (const auto& observable : observables_)
        {
            values.push_back(observable.expectation(result.state));
        }
        return values;
    }

    std::vector<double> values(outputQubits_.size(), 0.0);
    for (int outputIndex = 0; outputIndex < static_cast<int>(outputQubits_.size()); ++outputIndex)
    {
        if (outputQubits_[outputIndex] >= static_cast<int>(std::log2(result.state.size())))
        {
            throw std::out_of_range("QNN output qubit exceeds circuit width.");
        }
        for (std::size_t basis = 0; basis < probabilities.size(); ++basis)
        {
            if ((basis & (std::size_t{1} << outputQubits_[outputIndex])) != 0)
            {
                values[outputIndex] += probabilities[basis];
            }
        }
    }
    return values;
}

std::vector<std::vector<double>> QNN::backwardBatch(
    const std::vector<std::vector<double>>& batch,
    double shift) const
{
    std::vector<std::vector<double>> gradients;
    gradients.reserve(batch.size());

    for (const auto& features : batch)
    {
        gradients.push_back(backward(features, shift));
    }

    return gradients;
}
}