#include "circuit.hpp"
#include "circuit_canvas.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <QApplication>

#include <cmath>
#include <iomanip>
#include <iostream>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    quantum::circuit circuit;
    circuit.append(registers::q_register(3));
    const auto searchSpace = circuit(0);

    quantum::operators operations;
    operations.walshHadamard(searchSpace);

    constexpr int groverIterations = 2;
    for (int iteration = 0; iteration < groverIterations; ++iteration) {
        operations.x(searchSpace[1]);
        operations.h(searchSpace[2]);
        operations.ccx(searchSpace[2], searchSpace[0], searchSpace[1]);
        operations.h(searchSpace[2]);
        operations.x(searchSpace[1]);
        operations.amplitudeAmplification(searchSpace, 1);
    }
    operations.sampler(searchSpace, 1024);

    quantum::simulator simulator(circuit, operations, false);
    simulator.run();
    const auto& state = simulator.getStateVector();
    std::size_t mostLikely = 0;
    for (std::size_t basis = 1; basis < state.size(); ++basis) {
        if (std::norm(state[basis]) > std::norm(state[mostLikely])) {
            mostLikely = basis;
        }
    }
    std::cout << "Grover search (marked state |101>)\n"
              << "  Iterations: " << groverIterations << '\n'
              << "  Most likely result: |"
              << ((mostLikely & 4U) ? '1' : '0')
              << ((mostLikely & 2U) ? '1' : '0')
              << ((mostLikely & 1U) ? '1' : '0') << ">\n"
              << "  Success probability: " << std::fixed << std::setprecision(4)
              << std::norm(state[mostLikely]) << '\n'
              << "Simulation duration: " << simulator.duration().count() << " ms"
              << std::endl;

    app::circuit_canvas canvas(circuit, operations, QStringLiteral("Grover Search"));
    canvas.setWindowTitle(QStringLiteral("Grover Search Circuit"));
    canvas.resize(1200, 620);
    canvas.draw();
    canvas.show();
    return application.exec();
}
