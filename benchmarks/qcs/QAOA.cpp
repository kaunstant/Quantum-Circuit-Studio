#include "ansatz.hpp"
#include "optimizer.hpp"

#include <chrono>
#include <complex>
#include <iostream>
#include <vector>

int main()
{
    using StateVector = std::vector<std::complex<double>>;
    const auto wallStart = std::chrono::steady_clock::now();
    quantum::variational::ansatz qaoa(
        2,
        2,
        [](quantum::operators& operations, const std::vector<double>& parameters) {
            operations.h(0);
            operations.h(1);
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
    quantum::variational::approximator(300, qaoa, negativeExpectedCut, 1e-7);
    const auto wallFinish = std::chrono::steady_clock::now();

    std::cout << "workload=QAOA, graph=2_vertices_1_edge, depth=1, max_iterations=300\n"
              << "expected_cut=" << -qaoa.getOptimalEnergy() << '\n'
              << "iterations=" << qaoa.getTotalIterations() << '\n'
              << "end_to_end_wall_time_ms="
              << std::chrono::duration<double, std::milli>(
                     wallFinish - wallStart).count()
              << '\n';
}
