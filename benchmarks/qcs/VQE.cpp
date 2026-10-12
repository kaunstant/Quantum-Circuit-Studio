#include "ansatz.hpp"
#include "costfxn.hpp"
#include "optimizer.hpp"

#include <chrono>
#include <iostream>

int main()
{
    const auto wallStart = std::chrono::steady_clock::now();
    quantum::variational::ansatz ansatz(2, 2);
    quantum::costfxn::Hamiltonian hamiltonian;
    hamiltonian.append(-1.052373245772859, "II");
    hamiltonian.append(0.39793742484318045, "ZI");
    hamiltonian.append(-0.39793742484318045, "IZ");
    hamiltonian.append(-0.01128010425623538, "ZZ");
    hamiltonian.append(0.18093119978423156, "XX");

    quantum::variational::eigensolver(300, 1e-7, ansatz, hamiltonian);
    const auto wallFinish = std::chrono::steady_clock::now();

    std::cout << "workload=VQE, wires=2, Hamiltonian_terms=5, max_iterations=300\n"
              << "ground_state_energy=" << ansatz.getOptimalEnergy() << '\n'
              << "iterations=" << ansatz.getTotalIterations() << '\n'
              << "end_to_end_wall_time_ms="
              << std::chrono::duration<double, std::milli>(
                     wallFinish - wallStart).count()
              << '\n';
}
