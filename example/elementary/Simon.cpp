#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <iostream>

int main()
{
    quantum::circuit circuit;
    registers::q_register input(2);
    registers::q_register output(1);
    circuit.append(input);
    circuit.append(output);

    const auto inputRegister = circuit(0);
    const auto outputQubit = circuit(1)[0];
    quantum::operators operations;
    operations.h(inputRegister[0]);
    operations.h(inputRegister[1]);

    // For secret s = 11, f(x0,x1) = x0 XOR x1 satisfies f(x) = f(x XOR s).
    operations.cx(outputQubit, inputRegister[0]);
    operations.cx(outputQubit, inputRegister[1]);

    operations.h(inputRegister[0]);
    operations.h(inputRegister[1]);
    operations.sampler(inputRegister, 512);

    quantum::simulator simulator(circuit, operations);
    simulator.run();

    std::cout << "\nSimon's algorithm simulation duration: "
              << simulator.duration().count() << " ms\n";
}
