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
    registers::q_register reg(3);
    circuit.append(reg);

    quantum::operators operations;
    operations.x(circuit(0)[0]);
    operations.x(circuit(0)[1]);
    operations.qft(circuit(0));

    quantum::simulator simulator(circuit, operations, false);
    simulator.run();
    const auto wallFinish = std::chrono::steady_clock::now();

    std::cout << "workload=QFT, wires=3\n"
              << "end_to_end_wall_time_ms="
              << std::chrono::duration<double, std::milli>(
                     wallFinish - wallStart).count()
              << '\n'
              << "simulator_duration_ms=" << simulator.duration().count() << '\n';
}
