#include "ansatz.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace quantum::variational {

// Constructor for custom circuits without variational parameters
ansatz::ansatz(std::size_t numQubits, VariationalCircuit circuitBuilder)
    : quantum::circuit(numQubits),
      m_builder(std::move(circuitBuilder))
{
    if (numQubits == 0) {
        throw std::invalid_argument("A variational circuit must contain at least one qubit");
    }
    if (!m_builder) {
        throw std::invalid_argument("A custom variational circuit requires a builder");
    }
}

// Constructor for custom circuits with an explicit parameter count
ansatz::ansatz(
    std::size_t numQubits,
    std::size_t numParameters,
    VariationalCircuit circuitBuilder)
    : quantum::circuit(numQubits),
      m_paramCount(numParameters),
      m_initialParameters(defaultInitialParameters(numParameters)),
      m_builder(std::move(circuitBuilder))
{
    if (numQubits == 0) {
        throw std::invalid_argument("A variational circuit must contain at least one qubit");
    }
    if (!m_builder) {
        throw std::invalid_argument("A custom variational circuit requires a builder");
    }
}

// Constructor for the default hardware-efficient circuit using a layer count
ansatz::ansatz(std::size_t numQubits, std::size_t layersCount)
    : quantum::circuit(numQubits),
      m_paramCount(defaultParameterCount(numQubits, layersCount)),
      m_initialParameters(defaultInitialParameters(m_paramCount)),
      m_layersCount(layersCount)
{
    if (numQubits == 0 || layersCount == 0) {
        throw std::invalid_argument(
            "A hardware-efficient variational circuit requires qubits and layers");
    }
}

// Constructor for the default hardware-efficient circuit using initial parameters
ansatz::ansatz(std::size_t numQubits, const std::vector<double>& initialParams)
    : quantum::circuit(numQubits),
      m_paramCount(initialParams.size()),
      m_initialParameters(initialParams)
{
    if (numQubits == 0) {
        throw std::invalid_argument("A hardware-efficient variational circuit requires qubits");
    }
    if (initialParams.empty()) {
        throw std::invalid_argument("Initial parameters cannot be empty");
    }
    if (initialParams.size() % (2 * numQubits) != 0) {
        throw std::invalid_argument("Initial parameter size must match multiples of (2 * qubits)");
    }
    for (const double parameter : initialParams) {
        if (!std::isfinite(parameter)) {
            throw std::invalid_argument("Initial parameters must be finite");
        }
    }
    m_layersCount = initialParams.size() / (2 * numQubits);
}

// Returns the total number of parameters required by this circuit structure
std::size_t ansatz::getParamCount() const noexcept
{
    return m_paramCount;
}

// Returns the initial parameter vector used by an optimizer
const std::vector<double>& ansatz::getInitialParameters() const noexcept
{
    return m_initialParameters;
}

// Generates the operations timeline using the supplied parameter vector
void ansatz::generateOperators(
    quantum::operators& targetOperations,
    const std::vector<double>& parameters) const
{
    if (parameters.size() != m_paramCount) {
        throw std::invalid_argument(
            "Parameter vector size does not match the variational circuit specifications");
    }
    for (const double parameter : parameters) {
        if (!std::isfinite(parameter)) {
            throw std::invalid_argument("Variational parameters must be finite");
        }
    }

    if (m_builder) {
        m_builder(targetOperations, parameters);
        return;
    }
    buildHEansatz(targetOperations, parameters);
}

// Getter for the best energy score found
double ansatz::getOptimalEnergy() const noexcept
{
    return m_optimalEnergy;
}

// Getter for the best parameter angles found
const std::vector<double>& ansatz::getOptimalParameters() const noexcept
{
    return m_optimalParameters;
}

// Getter for the historical energy values logged
const std::vector<double>& ansatz::getEnergyHistory() const noexcept
{
    return m_energyHistory;
}

// Getter for the historic parameter trajectories logged
const std::vector<std::vector<double>>& ansatz::getParameterTrajectory() const noexcept
{
    return m_parameterTrajectory;
}

// Getter for total execution steps run
std::size_t ansatz::getTotalIterations() const noexcept
{
    return m_energyHistory.size();
}

// Calculates parameters needed for the hardware-efficient layers
std::size_t ansatz::defaultParameterCount(std::size_t qubits, std::size_t layers)
{
    if (qubits == 0 || layers == 0 ||
        qubits > std::numeric_limits<std::size_t>::max() / 2 / layers) {
        throw std::invalid_argument("Invalid hardware-efficient circuit size");
    }
    return 2 * qubits * layers;
}

// Creates a deterministic initial parameter vector
std::vector<double> ansatz::defaultInitialParameters(std::size_t count)
{
    std::vector<double> parameters(count);
    for (std::size_t index = 0; index < count; ++index) {
        parameters[index] = 0.2 + 0.17 * static_cast<double>(index);
    }
    return parameters;
}

// Builds the fallback default hardware-efficient circuit
void ansatz::buildHEansatz(
    quantum::operators& targetOperations,
    const std::vector<double>& parameters) const
{
    std::size_t parameter = 0;
    for (std::size_t layer = 0; layer < m_layersCount; ++layer) {
        for (std::size_t wire = 0; wire < numQubits(); ++wire) {
            targetOperations.ry(static_cast<int>(wire), parameters[parameter++]);
            targetOperations.rz(static_cast<int>(wire), parameters[parameter++]);
        }
        for (std::size_t wire = 0; wire + 1 < numQubits(); ++wire) {
            targetOperations.cx(static_cast<int>(wire + 1), static_cast<int>(wire));
        }
    }
}

// Resets tracking arrays before running a new optimization loop
void ansatz::resetOptimizationHistory()
{
    m_optimalEnergy = std::numeric_limits<double>::infinity();
    m_optimalParameters.clear();
    m_energyHistory.clear();
    m_parameterTrajectory.clear();
}

// Saves a single loop iteration score and parameter track entry
void ansatz::recordOptimizationStep(
    double energy,
    const std::vector<double>& parameters)
{
    m_energyHistory.push_back(energy);
    m_parameterTrajectory.push_back(parameters);
    if (energy < m_optimalEnergy) {
        m_optimalEnergy = energy;
        m_optimalParameters = parameters;
    }
}

} // namespace quantum::variational
