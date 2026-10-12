#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <iostream>

int main()
{
    quantum::circuit circuit;
    registers::q_register input(1);
    registers::q_register output(1);
    circuit.append(input);
    circuit.append(output);

    quantum::operators operations;
    operations.x(circuit(1)[0]);
    operations.h(circuit(0)[0]);
    operations.h(circuit(1)[0]);

    // Balanced oracle f(x) = x.
    operations.cx(circuit(1)[0], circuit(0)[0]);
    operations.h(circuit(0)[0]);
    operations.sampler(circuit(0), 128);

    quantum::simulator simulator(circuit, operations);
    simulator.run();

    std::cout << "\nDeutsch simulation duration: "
              << simulator.duration().count() << " ms\n";
}
