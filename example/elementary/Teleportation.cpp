#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <iostream>

int main()
{
    quantum::circuit circuit;
    registers::q_register wires(3);
    circuit.append(wires);

    const auto reg = circuit(0);
    quantum::operators operations;

    // Prepare an example input state |psi> on wire 0.
    operations.ry(reg[0], 1.0471975511965976);
    operations.rz(reg[0], 0.6283185307179586);

    // Create an entangled pair on wires 1 and 2.
    operations.h(reg[1]);
    operations.cx(reg[2], reg[1]);

    // Coherent Bell-basis decoding and conditional corrections.
    operations.cx(reg[1], reg[0]);
    operations.h(reg[0]);
    operations.cx(reg[2], reg[1]);
    operations.cz(reg[2], reg[0]);

    operations.sampler(reg, 1024);
    quantum::simulator simulator(circuit, operations);
    simulator.run();

    std::cout << "\nTeleportation simulation duration: "
              << simulator.duration().count() << " ms\n";
}
