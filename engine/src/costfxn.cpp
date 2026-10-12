#include "costfxn.hpp"

#include <cmath>
#include <complex>
#include <limits>
#include <stdexcept>
#include <utility>

namespace quantum::costfxn {

// Adds a Pauli term to the Hamiltonian after checking inputs
void Hamiltonian::append(double coefficient, const std::string& pauliString)
{
    if (!std::isfinite(coefficient)) {
        throw std::invalid_argument("Hamiltonian coefficient must be finite");
    }
    if (pauliString.empty()) {
        throw std::invalid_argument("Hamiltonian Pauli string cannot be empty");
    }
    for (const char pauli : pauliString) {
        if (pauli != 'I' && pauli != 'X' && pauli != 'Y' && pauli != 'Z') {
            throw std::invalid_argument("Hamiltonian Pauli strings may contain only I, X, Y, and Z");
        }
    }
    if (!m_terms.empty() && pauliString.size() != m_numQubits) {
        throw std::invalid_argument(
            "Each Hamiltonian Pauli string must have one character per qubit");
    }
    m_numQubits = pauliString.size();
    m_terms.push_back({coefficient, pauliString});
}

// Returns the list of stored Pauli terms
const std::vector<PauliTerm>& Hamiltonian::terms() const noexcept
{
    return m_terms;
}

// Returns the number of qubits described by the Hamiltonian terms
std::size_t Hamiltonian::numQubits() const noexcept
{
    return m_numQubits;
}

// Calculates the expectation value <Psi|H|Psi> from a state vector
double Hamiltonian::expectation(const StateVector& stateVector) const
{
    if (m_terms.empty()) {
        throw std::logic_error("Cannot evaluate an empty Hamiltonian");
    }
    if (m_numQubits >= std::numeric_limits<std::size_t>::digits ||
        stateVector.size() != (std::size_t{1} << m_numQubits)) {
        throw std::invalid_argument("State-vector size does not match the Hamiltonian qubit count");
    }

    // Calculates the normalization factor of the state vector
    double stateNorm = 0.0;
    for (const auto& amplitude : stateVector) {
        const double probability = std::norm(amplitude);
        if (!std::isfinite(probability)) {
            throw std::invalid_argument("State vector contains a non-finite amplitude");
        }
        stateNorm += probability;
    }
    if (!std::isfinite(stateNorm) || stateNorm <= 0.0) {
        throw std::invalid_argument("State vector has zero or invalid norm");
    }

    // Loops through each Pauli term and sums up their matrix overlap values
    std::complex<double> totalExpectation{0.0, 0.0};
    for (const PauliTerm& term : m_terms) {
        std::complex<double> termExpectation{0.0, 0.0};
        for (std::size_t basis = 0; basis < stateVector.size(); ++basis) {
            std::size_t mappedBasis = basis;
            std::complex<double> phase{1.0, 0.0};
            for (std::size_t wire = 0; wire < m_numQubits; ++wire) {
                const std::size_t mask = std::size_t{1} << wire;
                const bool bitIsOne = (basis & mask) != 0;
                switch (term.operators[wire]) {
                case 'I':
                    break;
                case 'X':
                    mappedBasis ^= mask;
                    break;
                case 'Y':
                    mappedBasis ^= mask;
                    phase *= bitIsOne
                        ? std::complex<double>{0.0, -1.0}
                        : std::complex<double>{0.0, 1.0};
                    break;
                case 'Z':
                    if (bitIsOne) {
                        phase = -phase;
                    }
                    break;
                }
            }
            termExpectation +=
                std::conj(stateVector[mappedBasis]) * phase * stateVector[basis];
        }
        totalExpectation += term.coefficient * termExpectation;
    }

    // Checks that Hermitian Hamiltonian terms produce a real expectation value
    const double imaginaryTolerance =
        1e-10 * (1.0 + std::abs(totalExpectation.real()));
    if (std::abs(totalExpectation.imag()) > imaginaryTolerance) {
        throw std::runtime_error("Hermitian Hamiltonian produced a complex expectation value");
    }
    return totalExpectation.real() / stateNorm;
}

// Calculates the expectation value of a Hamiltonian for a state vector
double expectation(const Hamiltonian& hamiltonian, const StateVector& stateVector)
{
    return hamiltonian.expectation(stateVector);
}

// Creates a reusable cost function from a Hamiltonian definition
CostFunction makeHamiltonianCostFunction(Hamiltonian hamiltonian)
{
    return [hamiltonian = std::move(hamiltonian)](const StateVector& stateVector) {
        return hamiltonian.expectation(stateVector);
    };
}

} // namespace quantum::costfxn
