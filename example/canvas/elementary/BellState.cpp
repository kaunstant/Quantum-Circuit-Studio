#include "circuit.hpp"
#include "circuit_canvas.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <QApplication>

#include <iomanip>
#include <iostream>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    quantum::circuit circuit;
    circuit.append(registers::q_register(2));

    quantum::operators operations;
    operations.h(circuit(0)[0]);
    operations.cx(circuit(0)[1], circuit(0)[0]);
    operations.sampler(circuit(0), 1024);

    quantum::simulator simulator(circuit, operations, false);
    simulator.run();
    std::cout << "Bell-state outcome probabilities\n" << std::fixed << std::setprecision(4);
    const auto& state = simulator.getStateVector();
    for (std::size_t basis = 0; basis < state.size(); ++basis) {
        const double probability = std::norm(state[basis]);
        if (probability > 1e-9) {
            std::cout << "  |" << ((basis & 2U) ? '1' : '0')
                      << ((basis & 1U) ? '1' : '0') << "> : "
                      << probability << '\n';
        }
    }
    std::cout << "Simulation duration: " << simulator.duration().count() << " ms\n"
              << std::flush;

    app::circuit_canvas canvas(circuit, operations, QStringLiteral("Bell State"));
    canvas.draw();
    canvas.setWindowTitle(QStringLiteral("Bell State Circuit"));
    canvas.resize(1000, 480);
    canvas.show();
    return application.exec();
}
