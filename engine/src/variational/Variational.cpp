#include "Variational.hpp"

#include <cmath>
#include <stdexcept>

namespace Variational
{
// =============================
// Hamiltonians and expectation values
// =============================

Hamiltonian::Hamiltonian(std::vector<PauliTerm> terms)
    : terms_(std::move(terms))
{
    if (terms_.empty())
        throw std::invalid_argument("A Hamiltonian must contain at least one term.");
}

double Hamiltonian::expectation(const std::vector<std::complex<double>>& state) const
{
    const int qubitCount = static_cast<int>(std::log2(state.size()));
    double result = 0.0;

    for (const PauliTerm& term : terms_)
    {
        if (static_cast<int>(term.paulis.size()) != qubitCount)
            throw std::invalid_argument("Pauli term width does not match the state.");

        std::complex<double> termValue = 0.0;
        for (std::size_t basis = 0; basis < state.size(); ++basis)
        {
            std::size_t source = basis;
            std::complex<double> phase = 1.0;

            for (int qubit = 0; qubit < qubitCount; ++qubit)
            {
                const bool bit = (basis & (std::size_t{1} << qubit)) != 0;
                const char pauli = term.paulis[qubitCount - qubit - 1];

                if (pauli == 'X')
                    source ^= std::size_t{1} << qubit;
                else if (pauli == 'Y')
                {
                    source ^= std::size_t{1} << qubit;
                    phase *= bit ? std::complex<double>(0.0, 1.0) : std::complex<double>(0.0, -1.0);
                }
                else if (pauli == 'Z' && bit)
                    phase = -phase;
                else if (pauli != 'I' && pauli != 'Z')
                    throw std::invalid_argument("Unsupported Pauli operator.");
            }

            termValue += std::conj(state[basis]) * phase * state[source];
        }

        result += term.coefficient * termValue.real();
    }

    return result;
}

const std::vector<PauliTerm>& Hamiltonian::terms() const
{
    return terms_;
}

CoordinateDescent::CoordinateDescent(double initialStep, int maxIterations)
    : initialStep_(initialStep), maxIterations_(maxIterations)
{
    if (initialStep <= 0.0 || maxIterations <= 0)
        throw std::invalid_argument("Optimizer settings must be positive.");
}

// =============================
// Optimizer
// =============================

OptimizationResult CoordinateDescent::minimize(
    const std::function<double(const std::vector<double>&)>& cost,
    std::vector<double> parameters) const
{
    OptimizationResult result{parameters, cost(parameters), 0};
    double step = initialStep_;

    for (int iteration = 0; iteration < maxIterations_; ++iteration)
    {
        bool improved = false;
        for (std::size_t index = 0; index < result.parameters.size(); ++index)
        {
            std::vector<double> trial = result.parameters;
            trial[index] += step;
            double trialValue = cost(trial);

            if (trialValue < result.value)
                improved = true;
            else
            {
                trial[index] -= 2.0 * step;
                trialValue = cost(trial);
                if (trialValue < result.value)
                    improved = true;
            }

            if (improved && trialValue < result.value)
            {
                result.parameters = trial;
                result.value = trialValue;
            }
        }

        result.iterations = iteration + 1;
        if (!improved)
            step *= 0.5;
        if (step < 1e-6)
            break;
    }

    return result;
}

VQE::VQE(ParameterizedCircuit ansatz, Hamiltonian hamiltonian)
    : ansatz_(std::move(ansatz)), hamiltonian_(std::move(hamiltonian))
{
}

// =============================
// VQE
// =============================

double VQE::energy(const std::vector<double>& parameters) const
{
    QuantumSimulator simulator;
    const SimulationResult result = simulator.run(ansatz_(parameters));
    return hamiltonian_.expectation(result.state);
}

OptimizationResult VQE::minimize(
    std::vector<double> initialParameters,
    const CoordinateDescent& optimizer) const
{
    return optimizer.minimize(
        [this](const std::vector<double>& parameters) { return energy(parameters); },
        std::move(initialParameters));
}

QAOA::QAOA(int qubitCount, std::vector<Edge> edges)
    : qubitCount_(qubitCount), edges_(std::move(edges))
{
    if (qubitCount <= 0)
        throw std::invalid_argument("QAOA requires at least one qubit.");
    for (const Edge& edge : edges_)
        if (edge.first < 0 || edge.second < 0 || edge.first >= qubitCount || edge.second >= qubitCount || edge.first == edge.second)
            throw std::invalid_argument("QAOA edge is invalid.");
}

// =============================
// QAOA
// =============================

QuantumCircuit QAOA::circuit(const std::vector<double>& parameters) const
{
    if (parameters.size() % 2 != 0 || parameters.empty())
        throw std::invalid_argument("QAOA parameters must contain gamma and beta pairs.");

    QuantumCircuit circuit(qubitCount_);
    for (int qubit = 0; qubit < qubitCount_; ++qubit)
        circuit.h(qubit);

    const std::size_t layerCount = parameters.size() / 2;
    for (std::size_t layer = 0; layer < layerCount; ++layer)
    {
        const double gamma = parameters[2 * layer];
        const double beta = parameters[2 * layer + 1];

        for (const Edge& edge : edges_)
        {
            const std::complex<double> positivePhase = std::exp(std::complex<double>(0.0, -gamma));
            const std::complex<double> negativePhase = std::exp(std::complex<double>(0.0, gamma));
            const Matrix zzPhase = {
                {positivePhase, 0.0, 0.0, 0.0},
                {0.0, negativePhase, 0.0, 0.0},
                {0.0, 0.0, negativePhase, 0.0},
                {0.0, 0.0, 0.0, positivePhase}
            };
            circuit.apply(zzPhase, {edge.first, edge.second});
        }

        for (int qubit = 0; qubit < qubitCount_; ++qubit)
            circuit.rx(qubit, 2.0 * beta);
    }

    return circuit;
}

double QAOA::cost(const std::vector<double>& parameters) const
{
    QuantumSimulator simulator;
    const SimulationResult result = simulator.run(circuit(parameters));
    const std::vector<double> probabilities = QuantumSimulator::probabilities(result.state);
    double expectedCut = 0.0;

    for (std::size_t basis = 0; basis < probabilities.size(); ++basis)
    {
        double cutValue = 0.0;
        for (const Edge& edge : edges_)
            if (((basis >> edge.first) & 1U) != ((basis >> edge.second) & 1U))
                cutValue += 1.0;
        expectedCut += probabilities[basis] * cutValue;
    }

    return -expectedCut;
}

OptimizationResult QAOA::minimize(
    std::vector<double> initialParameters,
    const CoordinateDescent& optimizer) const
{
    return optimizer.minimize(
        [this](const std::vector<double>& parameters) { return cost(parameters); },
        std::move(initialParameters));
}
}