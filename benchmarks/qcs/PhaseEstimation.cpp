#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"
#include "utils/quantum_helpers.hpp"

#include <chrono>
#include <iostream>

int main()
{
    const auto wallStart = std::chrono::steady_clock::now();
    quantum::circuit circuit;
    registers::q_register phaseRegister(3);
    registers::q_register eigenstate(1);
    circuit.append(phaseRegister);
    circuit.append(eigenstate);

    quantum::operators operations;
    operations.x(circuit(1)[0]);
    operations.h(circuit(1)[0]);
    quantum::utils::unrollQuantumPhaseEstimation(
        operations,
        circuit(0),
        circuit(1),
        [](quantum::operators& timeline,
           const quantum::qubit_view& control,
           const quantum::register_view& target,
           std::size_t power) {
            if ((power & 1U) != 0) {
                timeline.cx(target[0], control);
            }
        });
    operations.sampler(circuit(0), 256);

    quantum::simulator simulator(circuit, operations, false);
    simulator.run();
    const auto wallFinish = std::chrono::steady_clock::now();

    std::cout << "workload=Phase estimation, phase_wires=3, shots=256\n"
              << "end_to_end_wall_time_ms="
              << std::chrono::duration<double, std::milli>(
                     wallFinish - wallStart).count()
              << '\n'
              << "simulator_duration_ms=" << simulator.duration().count() << '\n';
}
