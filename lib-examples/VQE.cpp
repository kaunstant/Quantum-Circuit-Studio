#include <QCS>
#include "VariationalExampleSupport.hpp"

int main()
{
    const Variational::Hamiltonian hamiltonian({{1.0, "Z"}});
    const Variational::ParameterizedCircuit ansatz = [](const std::vector<double>& parameters) { QuantumCircuit circuit(1); circuit.ry(0, parameters.at(0)); return circuit; };
    Variational::VQE vqe(ansatz, hamiltonian);
    const std::vector<double> initialParameters = {0.4};
    const Variational::OptimizationResult result = vqe.minimize(initialParameters, Variational::CoordinateDescent(0.4, 100));
    printOptimizationResult("VQE: ground state of H = Z", initialParameters, result, "Energy");
    return 0;
}