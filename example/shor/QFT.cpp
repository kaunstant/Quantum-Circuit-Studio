#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <iostream>

int main()
{
    quantum::circuit circuit;
    registers::q_register reg(3);
    circuit.append(reg);

    const auto q = circuit(0);
    quantum::operators operations;
    operations.x(q[0]);
    operations.x(q[1]);
    operations.qft(circuit(0));

    quantum::simulator simulator(circuit, operations);
    simulator.run();
    simulator.printStateVector();

    std::cout << "\nQFT simulation duration: "
              << simulator.duration().count() << " ms\n";
}
