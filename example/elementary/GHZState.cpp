#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <iostream>

int main()
{
    quantum::circuit circuit;
    registers::q_register ghzRegister(3);
    circuit.append(ghzRegister);

    const auto reg = circuit(0);
    quantum::operators operations;
    operations.h(reg[0]);
    operations.cx(reg[1], reg[0]);
    operations.cx(reg[2], reg[1]);
    operations.sampler(reg, 1024);

    quantum::simulator simulator(circuit, operations);
    simulator.run();

    std::cout << "\nGHZ-state simulation duration: "
              << simulator.duration().count() << " ms\n";
}
