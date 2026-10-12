#include "ansatz.hpp"
#include "optimizer.hpp"

#include <chrono>
#include <complex>
#include <iostream>
#include <vector>

int main()
{
    using StateVector = std::vector<std::complex<double>>;
    quantum::variational::ansatz qaoa(
        2,
        2,
        [](quantum::operators& operations, const std::vector<double>& parameters) {
            operations.h(0);
            operations.h(1);

            // One QAOA layer for a two-vertex, one-edge Max-Cut graph.
            operations.cx(1, 0);
            operations.rz(1, parameters[0]);
            operations.cx(1, 0);
            operations.rx(0, 2.0 * parameters[1]);
            operations.rx(1, 2.0 * parameters[1]);
        });

    const auto negativeExpectedCut = [](const StateVector& state) {
        double cutValue = 0.0;
        for (std::size_t basis = 0; basis < state.size(); ++basis) {
            const bool firstBit = (basis & 1U) != 0;
            const bool secondBit = (basis & 2U) != 0;
            if (firstBit != secondBit) {
                cutValue += std::norm(state[basis]);
            }
        }
        return -cutValue;
    };

    const auto start = std::chrono::steady_clock::now();
    quantum::variational::approximator(300, qaoa, negativeExpectedCut, 1e-7);
    const auto finish = std::chrono::steady_clock::now();

    std::cout << "QAOA expected cut value: " << -qaoa.getOptimalEnergy() << '\n'
              << "Iterations: " << qaoa.getTotalIterations() << '\n'
              << "Optimal parameters:";
    for (const double parameter : qaoa.getOptimalParameters()) {
        std::cout << ' ' << parameter;
    }
    std::cout << "\nOptimization wall time: "
              << std::chrono::duration<double, std::milli>(finish - start).count()
              << " ms\n";
}
