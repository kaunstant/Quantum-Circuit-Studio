#pragma once

#include <complex>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace quantum::costfxn {

using StateVector = std::vector<std::complex<double>>;
using CostFunction = std::function<double(const StateVector&)>;

struct PauliTerm {
    double coefficient;
    std::string operators;
};

class Hamiltonian {
public:
    // Adds a Pauli term to the Hamiltonian
    void append(double coefficient, const std::string& pauliString);

    // Returns the list of stored Pauli terms
    const std::vector<PauliTerm>& terms() const noexcept;

    // Calculates the expectation value <Psi|H|Psi> from a state vector
    double expectation(const StateVector& stateVector) const;

    // Returns the number of qubits described by the Hamiltonian terms
    std::size_t numQubits() const noexcept;

private:
    std::vector<PauliTerm> m_terms;
    std::size_t m_numQubits = 0;
};

// Calculates the expectation value of a Hamiltonian for a state vector
double expectation(const Hamiltonian& hamiltonian, const StateVector& stateVector);

// Creates a reusable cost function from a Hamiltonian definition
CostFunction makeHamiltonianCostFunction(Hamiltonian hamiltonian);

} // namespace quantum::costfxn
