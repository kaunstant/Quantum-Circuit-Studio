#include "circuit.hpp"
#include "circuit_canvas.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"
#include "utils/quantum_helpers.hpp"

#include <QApplication>

#include <cstdint>
#include <iostream>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    constexpr std::uint64_t numberToFactor = 15;
    constexpr std::uint64_t base = 2;
    quantum::circuit circuit;
    circuit.append(registers::q_register(4));
    circuit.append(registers::q_register(4));

    const auto phaseRegister = circuit(0);
    const auto workRegister = circuit(1);
    quantum::operators operations;
    operations.x(workRegister[0]);
    quantum::utils::unrollQuantumPhaseEstimation(
        operations,
        phaseRegister,
        workRegister,
        [](quantum::operators& timeline,
           const quantum::qubit_view& control,
           const quantum::register_view& target,
           std::size_t power) {
            const std::uint64_t multiplier =
                quantum::utils::modularPower(base, power, numberToFactor);
            timeline.applyControlledRegister(
                target,
                control,
                quantum::utils::modularMultiplicationMatrix(
                    target.size(),
                    multiplier,
                    numberToFactor));
        });
    operations.sampler(phaseRegister, 1024);

    quantum::simulator simulator(circuit, operations, false);
    simulator.run();
    const std::vector<double> phaseProbabilities =
        quantum::utils::calculateRegisterProbabilities(
            simulator.getStateVector(),
            phaseRegister);
    const std::uint64_t order = quantum::utils::deducePeriodFromPhaseDistribution(
        phaseProbabilities,
        base,
        numberToFactor,
        1000);

    std::cout << "Shor period finding: factor " << numberToFactor
              << " using base " << base << '\n';
    if (order == 0) {
        std::cout << "The phase sample did not reveal a usable order; run again.\n";
    } else if (order % 2 != 0) {
        std::cout << "Recovered order r = " << order
                  << " (odd order; choose another base and retry).\n";
    } else {
        const std::uint64_t halfPower =
            quantum::utils::modularPower(base, order / 2, numberToFactor);
        const std::uint64_t firstFactor =
            quantum::utils::gcd(halfPower - 1, numberToFactor);
        const std::uint64_t secondFactor =
            quantum::utils::gcd(halfPower + 1, numberToFactor);
        if (firstFactor <= 1 || secondFactor <= 1 ||
            firstFactor >= numberToFactor || secondFactor >= numberToFactor) {
            std::cout << "Recovered order r = " << order
                      << " did not yield nontrivial factors; choose another base and retry.\n";
        } else {
            std::cout << "Recovered order r = " << order << '\n'
                      << "Factors of " << numberToFactor << ": "
                      << firstFactor << " and " << secondFactor << '\n';
        }
    }
    std::cout << "Simulation duration: " << simulator.duration().count() << " ms\n"
              << std::flush;

    app::circuit_canvas canvas(circuit, operations, QStringLiteral("Shor Period-Finding Circuit"));
    canvas.setWindowTitle(QStringLiteral("Shor Circuit"));
    canvas.resize(1400, 720);
    canvas.draw();
    canvas.show();
    return application.exec();
}
