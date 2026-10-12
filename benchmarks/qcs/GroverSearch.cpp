#include "circuit.hpp"
#include "operators.hpp"
#include "q_register.hpp"
#include "simulator.hpp"

#include <chrono>
#include <iostream>

int main()
{
    const auto wallStart = std::chrono::steady_clock::now();
    quantum::circuit circuit;
    registers::q_register searchSpace(3);
    circuit.append(searchSpace);
    const auto q = circuit(0);

    quantum::operators operations;
    operations.walshHadamard(circuit(0));
    for (int iteration = 0; iteration < 2; ++iteration) {
        operations.x(q[1]);
        operations.h(q[2]);
        operations.ccx(q[2], q[0], q[1]);
        operations.h(q[2]);
        operations.x(q[1]);
        operations.amplitudeAmplification(circuit(0), 1);
    }
    operations.sampler(circuit(0), 1024);

    quantum::simulator simulator(circuit, operations, false);
    simulator.run();
    const auto wallFinish = std::chrono::steady_clock::now();

    std::cout << "workload=Grover search, wires=3, iterations=2, shots=1024\n"
              << "end_to_end_wall_time_ms="
              << std::chrono::duration<double, std::milli>(
                     wallFinish - wallStart).count()
              << '\n'
              << "simulator_duration_ms=" << simulator.duration().count() << '\n';
}
