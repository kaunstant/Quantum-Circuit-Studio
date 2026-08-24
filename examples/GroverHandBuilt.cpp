#include "ExampleSupport.hpp"

#include <iostream>

int main()
{
    QuantumCircuit circuit(3);
    for (int qubit = 0; qubit < 3; ++qubit) circuit.h(qubit);

    // Oracle for |101>: X on zero-pattern bits, phase kickback, then undo X.
    circuit.x(1); circuit.h(2); circuit.multiControlledX({0, 1}, 2); circuit.h(2); circuit.x(1);
    // Diffusion operator.
    for (int qubit = 0; qubit < 3; ++qubit) circuit.h(qubit);
    for (int qubit = 0; qubit < 3; ++qubit) circuit.x(qubit);
    circuit.h(2); circuit.multiControlledX({0, 1}, 2); circuit.h(2);
    for (int qubit = 0; qubit < 3; ++qubit) circuit.x(qubit);
    for (int qubit = 0; qubit < 3; ++qubit) circuit.h(qubit);

    const SimulationResult result = QuantumSimulator().run(circuit);
    printExampleResult("Hand-built Grover search", result);
    std::cout << "Marked-state probability: " << std::norm(result.state[5]) << "\n";
    std::cout << "The marked basis state is |101>, index 5.\n";
    return 0;
}