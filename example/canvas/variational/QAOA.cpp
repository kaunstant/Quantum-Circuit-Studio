#include "ansatz.hpp"
#include "circuit_canvas.hpp"
#include "operators.hpp"
#include "optimizer.hpp"
#include "simulator.hpp"

#include <QApplication>

#include <chrono>
#include <complex>
#include <iostream>
#include <vector>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    using StateVector = std::vector<std::complex<double>>;
    quantum::variational::ansatz qaoa(
        2,
        2,
        [](quantum::operators& operations, const std::vector<double>& parameters) {
            operations.h(0);
            operations.h(1);
            operations.cx(1, 0);
            operations.rz(1, parameters.at(0));
            operations.cx(1, 0);
            operations.rx(0, 2.0 * parameters.at(1));
            operations.rx(1, 2.0 * parameters.at(1));
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

    const auto optimizationStart = std::chrono::steady_clock::now();
    quantum::variational::approximator(300, qaoa, negativeExpectedCut, 1e-7);
    const auto optimizationFinish = std::chrono::steady_clock::now();

    const std::vector<double>& optimalParameters = qaoa.getOptimalParameters();
    quantum::operators operations;
    qaoa.generateOperators(operations, optimalParameters);
    quantum::simulator simulator(qaoa, operations, false);
    simulator.run();

    const auto& state = simulator.getStateVector();
    double expectedCut = 0.0;
    std::size_t mostLikely = 0;
    for (std::size_t basis = 0; basis < state.size(); ++basis) {
        const double probability = std::norm(state[basis]);
        const bool firstBit = (basis & 1U) != 0;
        const bool secondBit = (basis & 2U) != 0;
        if (firstBit != secondBit) {
            expectedCut += probability;
        }
        if (probability > std::norm(state[mostLikely])) {
            mostLikely = basis;
        }
    }

    std::cout << "QAOA Max-Cut result for a two-vertex, one-edge graph\n"
              << "  Expected cut value: " << expectedCut << '\n'
              << "  Most likely bitstring: |"
              << ((mostLikely & 2U) ? '1' : '0')
              << ((mostLikely & 1U) ? '1' : '0') << ">\n"
              << "  Optimizer iterations: " << qaoa.getTotalIterations()
              << "\n  Optimized parameters:";
    for (double parameter : optimalParameters) {
        std::cout << ' ' << parameter;
    }
    std::cout << "\n  Optimization duration: "
              << std::chrono::duration<double, std::milli>(
                     optimizationFinish - optimizationStart).count()
              << " ms\n"
              << "  Final-state simulation duration: "
              << simulator.duration().count() << " ms\n"
              << std::flush;

    app::circuit_canvas canvas(qaoa, operations, QStringLiteral("Optimized QAOA Max-Cut Ansatz"));
    canvas.setWindowTitle(QStringLiteral("QAOA Circuit"));
    canvas.resize(1300, 560);
    canvas.draw();
    canvas.show();
    return application.exec();
}
