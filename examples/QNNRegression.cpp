#include "QNN.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    QNN::QNN model;
    model.setFeatureMap(
        [](const std::vector<double>& features)
        {
            QuantumCircuit circuit(1);
            circuit.ry(0, features.at(0));
            return circuit;
        });
    model.setAnsatz(
        [](const std::vector<double>& parameters)
        {
            QuantumCircuit circuit(1);
            circuit.ry(0, parameters.at(0));
            return circuit;
        });
    model.setObservable(Variational::Hamiltonian({{1.0, "Z"}}));
    model.setOutputMode(QNN::OutputMode::Expectation);
    model.setParameters({0.6});

    const std::vector<std::vector<double>> inputs = {
        {0.0},
        {0.7},
        {1.4}
    };
    const std::vector<double> targets = {
        std::cos(0.0),
        std::cos(0.7),
        std::cos(1.4)
    };

    const double initialLoss =
        (model.loss(inputs[0], targets[0]) +
         model.loss(inputs[1], targets[1]) +
         model.loss(inputs[2], targets[2])) / 3.0;
    const Variational::OptimizationResult result =
        model.train(inputs, targets, 60, 0.15);

    std::cout << "QNN regression example\n";
    std::cout << "Initial mean squared error: "
              << std::fixed << std::setprecision(6)
              << initialLoss << "\n";
    std::cout << "Final mean squared error: "
              << result.value << "\n";
    std::cout << "Predictions after training\n";
    for (std::size_t index = 0; index < inputs.size(); ++index)
    {
        std::cout << "Input " << index
                  << ": prediction = " << model.forward(inputs[index])
                  << ", target = " << targets[index] << "\n";
    }

    return 0;
}