#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <iostream>

int main()
{
    quantum::circuit circuit;
    registers::q_register bellPair(2);
    circuit.append(bellPair);

    quantum::operators operations;
    operations.h(circuit(0)[0]);
    operations.cx(circuit(0)[1], circuit(0)[0]);
    operations.measure(circuit(0));
    operations.sampler(circuit(0), 1024);

    quantum::simulator simulator(circuit, operations);
    simulator.run();

    std::cout << "\nBell-state simulation duration: "
              << simulator.duration().count() << " ms\n";
}
