#pragma once

#include "circuit.hpp"
#include "operators.hpp"

#include <chrono>
#include <complex>
#include <stdexcept>
#include <vector>

namespace quantum {

// Thrown when an operation references wires or registers outside its circuit.
class circuit_operation_mismatch : public std::logic_error {
public:
    using std::logic_error::logic_error;
};

class simulator {
public:
    // Circuit and operation references are non-owning and must outlive the simulator.
    simulator(const circuit& circuit, const operators& operations, bool printOutput = true);

    // Validate the timeline, initialize |0...0>, and execute each operation in order.
    void run();

    // Expose the final state vector and timing from the most recent run.
    const std::vector<Complex>& getStateVector() const noexcept;
    void printStateVector() const;
    std::chrono::duration<double, std::milli> duration() const noexcept;

private:
    const circuit* m_circuit = nullptr;
    const operators* m_operators = nullptr;
    bool m_printOutput = true;
    std::vector<Complex> m_stateVector;
    std::chrono::duration<double, std::milli> m_duration{};

    // Check operation metadata against the current circuit and gate definitions.
    void validate(const circuit& circuit, const operators& operations) const;
    // Allocate the dense state vector and prepare the all-zero basis state.
    void initializeState();
};

} // namespace quantum
