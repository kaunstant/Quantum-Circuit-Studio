#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"
#include "utils/quantum_helpers.hpp"

#include <cstdint>
#include <iostream>

int main()
{
    constexpr std::uint64_t numberToFactor = 15;
    constexpr std::uint64_t base = 2;
    constexpr std::size_t phaseBits = 4;
    constexpr std::size_t targetBits = 4;

    quantum::circuit circuit;
    registers::q_register phaseRegister(phaseBits);
    registers::q_register workRegister(targetBits);
    circuit.append(phaseRegister);
    circuit.append(workRegister);

    quantum::operators operations;
    operations.x(circuit(1)[0]); // Prepare |1>, whose modular orbit has order four.
    quantum::utils::unrollQuantumPhaseEstimation(
        operations,
        circuit(0),
        circuit(1),
        [](quantum::operators& timeline,
           const quantum::qubit_view& control,
           const quantum::register_view& target,
           std::size_t power) {
            constexpr std::uint64_t modulus = numberToFactor;
            const std::uint64_t multiplier =
                quantum::utils::modularPower(base, power, modulus);
            const auto matrix =
                quantum::utils::modularMultiplicationMatrix(
                    target.size(),
                    multiplier,
                    modulus);
            timeline.applyControlledRegister(target, control, matrix);
        });
    operations.sampler(circuit(0), 1024);

    quantum::simulator simulator(circuit, operations);
    simulator.run();

    const std::vector<double> phaseProbabilities =
        quantum::utils::calculateRegisterProbabilities(
            simulator.getStateVector(),
            circuit(0));
    const std::uint64_t order = quantum::utils::deducePeriodFromPhaseDistribution(
        phaseProbabilities,
        base,
        numberToFactor);

    if (order == 0) {
        std::cout << "This phase sample did not reveal a usable order; run again.\n";
    } else {
        const std::uint64_t halfPower =
            quantum::utils::modularPower(base, order / 2, numberToFactor);
        const std::uint64_t firstFactor =
            quantum::utils::gcd(halfPower - 1, numberToFactor);
        const std::uint64_t secondFactor =
            quantum::utils::gcd(halfPower + 1, numberToFactor);
        if (order % 2 != 0 || firstFactor <= 1 || secondFactor <= 1 ||
            firstFactor >= numberToFactor || secondFactor >= numberToFactor) {
            std::cout << "The recovered order did not yield nontrivial factors; run again.\n";
        } else {
            std::cout << "Recovered order r = " << order << '\n'
                      << "Factors of " << numberToFactor << ": "
                      << firstFactor << " and " << secondFactor << '\n';
        }
    }

    std::cout << "\nShor-factoring simulation duration: "
              << simulator.duration().count() << " ms\n";
}
