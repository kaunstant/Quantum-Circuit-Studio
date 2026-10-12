#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <iostream>
#include <string>

int main()
{
    const std::string secret = "1011";
    quantum::circuit circuit;
    registers::q_register input(secret.size());
    registers::q_register output(1);
    circuit.append(input);
    circuit.append(output);

    const auto inputRegister = circuit(0);
    const auto outputRegister = circuit(1);
    quantum::operators operations;

    operations.x(outputRegister[0]);
    operations.h(outputRegister[0]);
    for (std::size_t bit = 0; bit < input.size(); ++bit) {
        operations.h(inputRegister[bit]);
        if (secret[secret.size() - bit - 1] == '1') {
            operations.cx(outputRegister[0], inputRegister[bit]);
        }
    }
    for (std::size_t bit = 0; bit < input.size(); ++bit) {
        operations.h(inputRegister[bit]);
    }
    operations.sampler(inputRegister, 128);

    quantum::simulator simulator(circuit, operations);
    simulator.run();

    std::cout << "\nBernstein-Vazirani simulation duration: "
              << simulator.duration().count() << " ms\n";
}
