#include <QCS>

#include <cmath>
#include <iostream>

int main()
{
    QNN::QNN model;
    model.setFeatureMap([](const std::vector<double>& features) { QuantumCircuit circuit(1); circuit.ry(0, features.at(0)); return circuit; });
    model.setAnsatz([](const std::vector<double>& parameters) { QuantumCircuit circuit(1); circuit.ry(0, parameters.at(0)); return circuit; });
    model.setObservable(Variational::Hamiltonian({{1.0, "Z"}}));
    model.setOutputMode(QNN::OutputMode::Expectation);
    model.setParameters({0.6});
    const std::vector<std::vector<double>> inputs = {{0.0}, {0.7}, {1.4}};
    const std::vector<double> targets = {std::cos(0.0), std::cos(0.7), std::cos(1.4)};
    const Variational::OptimizationResult result = model.train(inputs, targets, 60, 0.15);
    std::cout << "QNN regression final MSE: " << result.value << "\n";
    return 0;
}