#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <iostream>
#include <string>

int main()
{
    const std::string balancedMask = "101";
    quantum::circuit circuit;
    registers::q_register input(balancedMask.size());
    registers::q_register output(1);
    circuit.append(input);
    circuit.append(output);

    const auto inputRegister = circuit(0);
    const auto outputQubit = circuit(1)[0];
    quantum::operators operations;

    operations.x(outputQubit);
    operations.h(outputQubit);
    for (std::size_t bit = 0; bit < input.size(); ++bit) {
        operations.h(inputRegister[bit]);
    }

    // Balanced oracle f(x) = x dot 101 (mod 2).
    for (std::size_t bit = 0; bit < input.size(); ++bit) {
        if (balancedMask[balancedMask.size() - bit - 1] == '1') {
            operations.cx(outputQubit, inputRegister[bit]);
        }
    }
    for (std::size_t bit = 0; bit < input.size(); ++bit) {
        operations.h(inputRegister[bit]);
    }
    operations.measure(inputRegister);
    operations.sampler(inputRegister, 128);

    quantum::simulator simulator(circuit, operations);
    simulator.run();

    std::cout << "\nDeutsch-Jozsa simulation duration: "
              << simulator.duration().count() << " ms\n";
}
