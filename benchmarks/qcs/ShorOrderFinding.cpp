#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"
#include "utils/quantum_helpers.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>

int main()
{
    constexpr std::uint64_t numberToFactor = 15;
    constexpr std::uint64_t base = 2;
    constexpr std::size_t phaseBits = 4;
    constexpr std::size_t targetBits = 4;
    const auto wallStart = std::chrono::steady_clock::now();

    quantum::circuit circuit;
    registers::q_register phaseRegister(phaseBits);
    registers::q_register workRegister(targetBits);
    circuit.append(phaseRegister);
    circuit.append(workRegister);

    quantum::operators operations;
    operations.x(circuit(1)[0]);
    quantum::utils::unrollQuantumPhaseEstimation(
        operations,
        circuit(0),
        circuit(1),
        [](quantum::operators& timeline,
           const quantum::qubit_view& control,
           const quantum::register_view& target,
           std::size_t power) {
            const std::uint64_t multiplier =
                quantum::utils::modularPower(base, power, numberToFactor);
            const auto matrix =
                quantum::utils::modularMultiplicationMatrix(
                    target.size(), multiplier, numberToFactor);
            timeline.applyControlledRegister(target, control, matrix);
        });
    operations.sampler(circuit(0), 1024);

    quantum::simulator simulator(circuit, operations, false);
    simulator.run();
    const auto probabilities = quantum::utils::calculateRegisterProbabilities(
        simulator.getStateVector(), circuit(0));
    const std::uint64_t order =
        quantum::utils::deducePeriodFromPhaseDistribution(
            probabilities, base, numberToFactor);
    const auto wallFinish = std::chrono::steady_clock::now();

    std::cout << "workload=Shor order finding, N=15, a=2, phase_wires=4, shots=1024\n";
    if (order != 0 && order % 2 == 0) {
        const std::uint64_t halfPower =
            quantum::utils::modularPower(base, order / 2, numberToFactor);
        std::cout << "recovered_order=" << order
                  << "\nfactors="
                  << quantum::utils::gcd(halfPower - 1, numberToFactor)
                  << ','
                  << quantum::utils::gcd(halfPower + 1, numberToFactor)
                  << '\n';
    } else {
        std::cout << "recovered_order=none\n";
    }
    std::cout << "end_to_end_wall_time_ms="
              << std::chrono::duration<double, std::milli>(
                     wallFinish - wallStart).count()
              << '\n'
              << "simulator_duration_ms=" << simulator.duration().count() << '\n';
}
