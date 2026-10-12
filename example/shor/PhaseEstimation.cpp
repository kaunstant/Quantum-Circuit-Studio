#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"
#include "utils/quantum_helpers.hpp"

#include <iostream>

int main()
{
    quantum::circuit circuit;
    registers::q_register phaseRegister(3);
    registers::q_register eigenstate(1);
    circuit.append(phaseRegister);
    circuit.append(eigenstate);

    const auto target = circuit(1)[0];
    quantum::operators operations;
    operations.x(target);

    // X has eigenvalue -1 on |->. Prepare |-> before estimating its phase 1/2.
    operations.h(target);
    quantum::utils::unrollQuantumPhaseEstimation(
        operations,
        circuit(0),
        circuit(1),
        [](quantum::operators& timeline,
           const quantum::qubit_view& control,
           const quantum::register_view& targetRegister,
           std::size_t power) {
            if ((power & 1U) != 0) {
                timeline.cx(targetRegister[0], control);
            }
        });
    operations.sampler(circuit(0), 256);

    quantum::simulator simulator(circuit, operations);
    simulator.run();

    std::cout << "\nPhase-estimation simulation duration: "
              << simulator.duration().count() << " ms\n";
}
