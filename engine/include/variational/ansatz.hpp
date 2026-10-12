#pragma once

#include "circuit.hpp"
#include "operators.hpp"

#include <complex>
#include <cstddef>
#include <functional>
#include <limits>
#include <vector>

namespace quantum {
namespace costfxn {
class Hamiltonian;
}
}

namespace quantum::variational {

// VARIATIONAL ANSATZ (PARAMETRIC CIRCUIT)

class ansatz : public quantum::circuit {
public:
    // Callback function type used to build custom circuits using a parameters vector
    using VariationalCircuit =
        std::function<void(quantum::operators& ops, const std::vector<double>& params)>;

    // Constructor for custom user-defined circuits
    ansatz(std::size_t numQubits, VariationalCircuit circuit);
    ansatz(
        std::size_t numQubits,
        std::size_t numParameters,
        VariationalCircuit circuit);

    // Constructor that auto-generates a standard hardware-efficient circuit using a layers count
    ansatz(std::size_t numQubits, std::size_t layersCount);

    // Constructor that auto-generates a standard hardware-efficient circuit using specific starting parameters
    ansatz(std::size_t numQubits, const std::vector<double>& initialParams);

    // Returns the total number of parameters required by this circuit structure
    std::size_t getParamCount() const noexcept;

    // Returns the initial parameter vector used by an optimizer
    const std::vector<double>& getInitialParameters() const noexcept;

    // Generates the operations timeline using a specific set of parameters passed by reference
    void generateOperators(quantum::operators& targetOperations, const std::vector<double>& parameters) const;

    // Getters for Optimization Results
    double getOptimalEnergy() const noexcept;
    const std::vector<double>& getOptimalParameters() const noexcept;
    const std::vector<double>& getEnergyHistory() const noexcept;
    const std::vector<std::vector<double>>& getParameterTrajectory() const noexcept;
    std::size_t getTotalIterations() const noexcept;

private:
    // Friend optimizers can directly modify internal optimization metrics
    friend void eigensolver(
        std::size_t,
        double,
        ansatz&,
        const quantum::costfxn::Hamiltonian&);
    friend void approximator(
        std::size_t,
        ansatz&,
        std::function<double(const std::vector<std::complex<double>>&)>,
        double);

    std::size_t m_paramCount = 0;
    std::vector<double> m_initialParameters;
    VariationalCircuit m_builder;
    std::size_t m_layersCount = 0;

    // Optimization tracking metrics
    double m_optimalEnergy = std::numeric_limits<double>::infinity();
    std::vector<double> m_optimalParameters;
    std::vector<double> m_energyHistory;
    std::vector<std::vector<double>> m_parameterTrajectory;

    // Calculates parameters needed for the hardware-efficient layers
    static std::size_t defaultParameterCount(std::size_t qubits, std::size_t layers);
    static std::vector<double> defaultInitialParameters(std::size_t count);
    
    // Builds the fallback default hardware-efficient circuit
    void buildHEansatz(
        quantum::operators& targetOperations,
        const std::vector<double>& parameters) const;
        
    // Resets tracking arrays before running a new optimization loop
    void resetOptimizationHistory();
    
    // Saves a single loop iteration score and parameters track entry
    void recordOptimizationStep(double energy, const std::vector<double>& parameters);
};

} // namespace quantum::variational
