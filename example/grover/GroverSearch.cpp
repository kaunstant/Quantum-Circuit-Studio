#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <iostream>

int main()
{
    quantum::circuit circuit;
    registers::q_register searchSpace(3);
    circuit.append(searchSpace);

    const auto q = circuit(0);
    quantum::operators operations;
    operations.walshHadamard(circuit(0));

    // Each Grover iteration contains both the phase oracle for |101> and diffusion.
    for (int iteration = 0; iteration < 2; ++iteration) {
        operations.x(q[1]);
        operations.h(q[2]);
        operations.ccx(q[2], q[0], q[1]);
        operations.h(q[2]);
        operations.x(q[1]);
        operations.amplitudeAmplification(circuit(0), 1);
    }
    operations.sampler(circuit(0), 1024);

    quantum::simulator simulator(circuit, operations);
    simulator.run();

    std::cout << "\nGrover-search simulation duration: "
              << simulator.duration().count() << " ms\n";
}
