#include "optimizer.hpp"
#include "simulator.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace quantum::variational {
namespace {

// Internal Adam optimizer backend logic loop that minimizes a given objective function
template <typename Objective, typename RecordStep>
void minimizeAdam(
    std::size_t maxIterations,
    double convergenceTolerance,
    double learningRate,
    const std::vector<double>& initialParameters,
    Objective&& objective,
    RecordStep&& recordStep)
{
    if (!std::isfinite(convergenceTolerance) || convergenceTolerance <= 0.0 ||
        !std::isfinite(learningRate) || learningRate <= 0.0) {
        throw std::invalid_argument("Optimizer tolerance and learning rate must be positive and finite");
    }

    std::vector<double> parameters = initialParameters;
    std::vector<double> firstMoment(parameters.size(), 0.0);
    std::vector<double> secondMoment(parameters.size(), 0.0);
    double energy = objective(parameters);
    recordStep(energy, parameters);

    constexpr double betaOne = 0.9;
    constexpr double betaTwo = 0.999;
    constexpr double epsilon = 1e-8;
    constexpr double finiteDifferenceStep = 1e-5;

    // Optimization iteration loop tracking updates
    for (std::size_t iteration = 1; iteration <= maxIterations; ++iteration) {
        std::vector<double> gradient(parameters.size(), 0.0);
        double gradientNormSquared = 0.0;
        
        // Loops through each parameter to compute numerical gradients using finite difference
        for (std::size_t parameter = 0; parameter < parameters.size(); ++parameter) {
            std::vector<double> plus = parameters;
            std::vector<double> minus = parameters;
            plus[parameter] += finiteDifferenceStep;
            minus[parameter] -= finiteDifferenceStep;
            gradient[parameter] =
                (objective(plus) - objective(minus)) / (2.0 * finiteDifferenceStep);
            gradientNormSquared += gradient[parameter] * gradient[parameter];
        }

        std::vector<double> nextParameters(parameters.size());
        const double firstBiasCorrection =
            1.0 - std::pow(betaOne, static_cast<double>(iteration));
        const double secondBiasCorrection =
            1.0 - std::pow(betaTwo, static_cast<double>(iteration));
            
        // Updates moments and calculates the next set of variational parameters
        for (std::size_t parameter = 0; parameter < parameters.size(); ++parameter) {
            firstMoment[parameter] =
                betaOne * firstMoment[parameter] + (1.0 - betaOne) * gradient[parameter];
            secondMoment[parameter] =
                betaTwo * secondMoment[parameter] +
                (1.0 - betaTwo) * gradient[parameter] * gradient[parameter];
            const double correctedFirst = firstMoment[parameter] / firstBiasCorrection;
            const double correctedSecond = secondMoment[parameter] / secondBiasCorrection;
            nextParameters[parameter] =
                parameters[parameter] - learningRate * correctedFirst /
                    (std::sqrt(correctedSecond) + epsilon);
        }

        const double nextEnergy = objective(nextParameters);
        if (!std::isfinite(nextEnergy)) {
            throw std::runtime_error("Optimizer objective returned a non-finite value");
        }
        recordStep(nextEnergy, nextParameters);

        const double energyChange = std::abs(nextEnergy - energy);
        parameters = std::move(nextParameters);
        energy = nextEnergy;
        
        // Checks if the optimization criteria has converged to a local minimum
        if (energyChange <= convergenceTolerance &&
            std::sqrt(gradientNormSquared) <= std::sqrt(convergenceTolerance)) {
            break;
        }
    }
}

// Runs a circuit simulation on the ansatz blueprint using a specific parameters reference vector
double evaluateAnsatz(
    ansatz& targetAnsatz,
    const std::vector<double>& parameters,
    const quantum::costfxn::CostFunction& costFunction)
{
    quantum::operators operations;
    targetAnsatz.generateOperators(operations, parameters);
    quantum::simulator simulator(targetAnsatz, operations, false);
    simulator.run();
    
    const double cost = costFunction(simulator.getStateVector());
    if (!std::isfinite(cost)) {
        throw std::runtime_error("Variational cost function returned a non-finite value");
    }
    return cost;
}

} // namespace

// Runs the variational quantum eigensolver loop using a Hamiltonian
void eigensolver(
    std::size_t maxIterations,
    double convergenceTolerance,
    ansatz& targetAnsatz,
    const quantum::costfxn::Hamiltonian& hamiltonian)
{
    if (hamiltonian.terms().empty()) {
        throw std::logic_error("Eigensolver requires at least one Hamiltonian term");
    }
    if (hamiltonian.numQubits() != targetAnsatz.numQubits()) {
        throw std::invalid_argument(
            "Hamiltonian qubit count must match the variational circuit");
    }
    targetAnsatz.resetOptimizationHistory();

    const auto energy =
        [&hamiltonian](const quantum::costfxn::StateVector& state) {
            return quantum::costfxn::expectation(hamiltonian, state);
        };
    auto objective = [&targetAnsatz, &energy](const std::vector<double>& parameters) {
        return evaluateAnsatz(targetAnsatz, parameters, energy);
    };
    auto recordStep = [&targetAnsatz](
                          double value,
                          const std::vector<double>& parameters) {
        targetAnsatz.recordOptimizationStep(value, parameters);
    };

    minimizeAdam(
        maxIterations,
        convergenceTolerance,
        0.05,
        targetAnsatz.getInitialParameters(),
        objective,
        recordStep);
}

// Runs the combinatorial approximation loop using a graph cost function
void approximator(
    std::size_t maxIterations,
    ansatz& targetAnsatz,
    quantum::costfxn::CostFunction costFunction,
    double convergenceTolerance)
{
    if (!costFunction) {
        throw std::invalid_argument("Approximator requires a cost function");
    }
    targetAnsatz.resetOptimizationHistory();
    auto objective = [&targetAnsatz, &costFunction](
                         const std::vector<double>& parameters) {
        return evaluateAnsatz(targetAnsatz, parameters, costFunction);
    };
    auto recordStep = [&targetAnsatz](
                          double value,
                          const std::vector<double>& parameters) {
        targetAnsatz.recordOptimizationStep(value, parameters);
    };

    minimizeAdam(
        maxIterations,
        convergenceTolerance,
        0.05,
        targetAnsatz.getInitialParameters(),
        objective,
        recordStep);
}

} // namespace quantum::variational
