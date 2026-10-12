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
    registers::q_register bellPair(2);
    circuit.append(bellPair);

    quantum::operators operations;
    operations.h(circuit(0)[0]);
    operations.cx(circuit(0)[1], circuit(0)[0]);
    operations.sampler(circuit(0), 1024);

    quantum::simulator simulator(circuit, operations, false);
    simulator.run();
    const auto wallFinish = std::chrono::steady_clock::now();

    std::cout << "workload=Bell-state circuit, wires=2, shots=1024\n"
              << "end_to_end_wall_time_ms="
              << std::chrono::duration<double, std::milli>(
                     wallFinish - wallStart).count()
              << '\n'
              << "simulator_duration_ms=" << simulator.duration().count() << '\n';
}
