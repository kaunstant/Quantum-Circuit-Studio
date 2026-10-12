#include "ansatz.hpp"
#include "circuit_canvas.hpp"
#include "costfxn.hpp"
#include "operators.hpp"
#include "optimizer.hpp"
#include "simulator.hpp"

#include <QApplication>

#include <chrono>
#include <iostream>
#include <vector>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    quantum::variational::ansatz ansatz(2, 2);
    quantum::costfxn::Hamiltonian hamiltonian;
    hamiltonian.append(-1.052373245772859, "II");
    hamiltonian.append(0.39793742484318045, "ZI");
    hamiltonian.append(-0.39793742484318045, "IZ");
    hamiltonian.append(-0.01128010425623538, "ZZ");
    hamiltonian.append(0.18093119978423156, "XX");

    const auto optimizationStart = std::chrono::steady_clock::now();
    quantum::variational::eigensolver(300, 1e-7, ansatz, hamiltonian);
    const auto optimizationFinish = std::chrono::steady_clock::now();

    const std::vector<double>& optimalParameters = ansatz.getOptimalParameters();
    quantum::operators operations;
    ansatz.generateOperators(operations, optimalParameters);
    quantum::simulator simulator(ansatz, operations, false);
    simulator.run();
    const double energy = hamiltonian.expectation(simulator.getStateVector());

    std::cout << "VQE optimization result\n"
              << "  Ground-state energy estimate: " << energy << '\n'
              << "  Optimizer iterations: " << ansatz.getTotalIterations() << '\n'
              << "  Optimized parameters:";
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

    app::circuit_canvas canvas(ansatz, operations, QStringLiteral("Optimized VQE Ansatz"));
    canvas.setWindowTitle(QStringLiteral("VQE Circuit"));
    canvas.resize(1200, 560);
    canvas.draw();
    canvas.show();
    return application.exec();
}
