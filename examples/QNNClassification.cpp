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
    model.setParameters({0.35});
    model.setOutputMode(QNN::OutputMode::Probability);
    model.setOutputQubit(0);

    const std::vector<std::vector<double>> inputs = {
        {0.0},
        {std::acos(-1.0)}
    };
    const std::vector<double> predictions = model.forwardBatch(inputs);

    std::cout << "QNN classification example\n";
    std::cout << "Probability of class 1\n";
    for (std::size_t index = 0; index < predictions.size(); ++index)
    {
        std::cout << "Input " << index << ": "
                  << std::fixed << std::setprecision(6)
                  << predictions[index] << "\n";
    }

    const std::vector<double> gradient = model.backward(inputs[0]);
    std::cout << "Parameter-shift gradient at input 0: "
              << gradient.at(0) << "\n";
    std::cout << "Expected classes: 0, 1\n";
    return 0;
}