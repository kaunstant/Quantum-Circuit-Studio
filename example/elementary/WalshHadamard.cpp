#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <iostream>

int main()
{
    quantum::circuit circuit;
    registers::q_register reg(4);
    circuit.append(reg);

    quantum::operators operations;
    operations.walshHadamard(circuit(0));
    operations.sampler(circuit(0), 1024);

    quantum::simulator simulator(circuit, operations);
    simulator.run();

    std::cout << "\nWalsh-Hadamard simulation duration: "
              << simulator.duration().count() << " ms\n";
}
