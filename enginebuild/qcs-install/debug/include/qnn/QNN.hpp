#pragma once

#include "QuantumCircuit.hpp"
#include "QuantumSimulator.hpp"
#include "Variational.hpp"

#include <functional>
#include <vector>

namespace QNN
{
// =============================
// QNN output and circuit contracts
// =============================

enum class OutputMode
{
    Expectation,
    Probability
};

using FeatureMap = std::function<QuantumCircuit(const std::vector<double>&)>;
using Ansatz = std::function<QuantumCircuit(const std::vector<double>&)>;

struct ForwardResult
{
    double value = 0.0;
    std::vector<double> probabilities;
    std::vector<std::complex<double>> state;
};

class QNN
{
public:
    QNN();

    // Configure U(x, theta) as two composable circuit builders.
    void setFeatureMap(FeatureMap featureMap);
    void setAnsatz(Ansatz ansatz);
    void setParameters(std::vector<double> parameters);
    void setObservable(Variational::Hamiltonian observable);
    void setObservables(std::vector<Variational::Hamiltonian> observables);
    void setOutputMode(OutputMode mode);
    void setOutputQubit(int qubit);
    void setOutputQubits(std::vector<int> qubits);

    ForwardResult forwardPass(
        const std::vector<double>& features) const;
    double forward(const std::vector<double>& features) const;
    // Vector output supports multiple observables or probability qubits.
    std::vector<double> forwardValues(
        const std::vector<double>& features) const;
    std::vector<double> forwardBatch(
        const std::vector<std::vector<double>>& batch) const;
    // Parameter-shift differentiation for the configured scalar output.
    std::vector<double> backward(
        const std::vector<double>& features,
        double shift = 1.5707963267948966) const;
    std::vector<std::vector<double>> backwardBatch(
        const std::vector<std::vector<double>>& batch,
        double shift = 1.5707963267948966) const;

    double loss(
        const std::vector<double>& features,
        double target) const;
    std::vector<double> lossGradient(
        const std::vector<double>& features,
        double target) const;

    Variational::OptimizationResult train(
        const std::vector<std::vector<double>>& features,
        const std::vector<double>& targets,
        int iterations = 50,
        double learningRate = 0.1);

private:
    FeatureMap featureMap_;
    Ansatz ansatz_;
    std::vector<double> parameters_;
    Variational::Hamiltonian observable_;
    std::vector<Variational::Hamiltonian> observables_;
    OutputMode outputMode_;
    int outputQubit_;
    std::vector<int> outputQubits_;
    bool hasObservable_;

    QuantumCircuit buildCircuit(
        const std::vector<double>& features,
        const std::vector<double>& parameters) const;
    void validateReady() const;
};
}