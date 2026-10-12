#include "ansatz.hpp"
#include "costfxn.hpp"
#include "optimizer.hpp"

#include <chrono>
#include <iostream>

int main()
{
    quantum::variational::ansatz ansatz(2, 2);
    quantum::costfxn::Hamiltonian hamiltonian;
    hamiltonian.append(-1.052373245772859, "II");
    hamiltonian.append(0.39793742484318045, "ZI");
    hamiltonian.append(-0.39793742484318045, "IZ");
    hamiltonian.append(-0.01128010425623538, "ZZ");
    hamiltonian.append(0.18093119978423156, "XX");

    const auto start = std::chrono::steady_clock::now();
    quantum::variational::eigensolver(300, 1e-7, ansatz, hamiltonian);
    const auto finish = std::chrono::steady_clock::now();

    std::cout << "VQE energy: " << ansatz.getOptimalEnergy() << '\n'
              << "Iterations: " << ansatz.getTotalIterations() << '\n'
              << "Optimal parameters:";
    for (const double parameter : ansatz.getOptimalParameters()) {
        std::cout << ' ' << parameter;
    }
    std::cout << "\nOptimization wall time: "
              << std::chrono::duration<double, std::milli>(finish - start).count()
              << " ms\n";
}
