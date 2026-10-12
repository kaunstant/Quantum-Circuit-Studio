#include "circuit.hpp"
#include "circuit_canvas.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <QApplication>

#include <iostream>
#include <vector>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    quantum::circuit circuit(8);
    circuit.append(registers::q_register(2));

    const quantum::Matrix2D xMatrix = {
        {{0.0, 0.0}, {1.0, 0.0}},
        {{1.0, 0.0}, {0.0, 0.0}}
    };
    const quantum::Matrix2D registerXMatrix = {
        {{0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
        {{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
        {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}},
        {{0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}}
    };

    quantum::operators operations;
    operations.apply(circuit[0], xMatrix);
    operations.apply(circuit[1], xMatrix);

    operations.controlled(circuit[2], circuit[0], xMatrix);
    operations.controlled(circuit[3], circuit[1], xMatrix);

    operations.multi_controlled(4, {0, 1}, xMatrix);
    operations.multi_controlled(5, {0, 2}, xMatrix);
    operations.multi_controlled(6, {1, 3, 4}, xMatrix);

    operations.applyRegister(circuit(0), registerXMatrix);
    operations.applyControlledRegister(circuit(0), circuit[0], registerXMatrix);

    quantum::simulator simulator(circuit, operations, false);
    simulator.run();

    std::cout << "Custom and register operator examples\n"
              << "  Custom 2x2 matrix: apply()\n"
              << "  Controlled custom matrices: controlled()\n"
              << "  Multi-controlled custom matrices: multi_controlled()\n"
              << "  Custom register matrix: applyRegister()\n"
              << "  Controlled custom register matrix: applyControlledRegister()\n"
              << "Timeline operation count: " << operations.getGates().size() << '\n'
              << "Final state-vector norm: ";
    double norm = 0.0;
    for (const quantum::Complex amplitude : simulator.getStateVector()) {
        norm += std::norm(amplitude);
    }
    std::cout << norm << '\n'
              << "Simulation duration: " << simulator.duration().count() << " ms\n"
              << "Hover over the gates to inspect their names, controls, parameters, and matrix dimensions.\n"
              << std::flush;

    app::circuit_canvas canvas(circuit, operations, QStringLiteral("Custom Operator Gallery"));
    canvas.setWindowTitle(QStringLiteral("Custom and Register Operators"));
    canvas.resize(1600, 900);
    canvas.draw();
    canvas.show();
    return application.exec();
}
