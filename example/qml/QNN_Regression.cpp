#include "qnn.hpp"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
    quantum::variational::ansatz variationalCircuit(
        1,
        1,
        [](quantum::operators& operations, const std::vector<double>& weights) {
            operations.ry(0, weights[0]);
        });

    quantum::ml::qnn model(
        1,
        [](quantum::operators& operations, const std::vector<double>& features) {
            operations.ry(0, features.at(0));
        },
        variationalCircuit);

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
    const std::vector<double> initialWeights = {0.6};
    const double initialLoss = model.loss(inputs, initialWeights, targets);

    const auto start = std::chrono::steady_clock::now();
    const std::vector<double> trainedWeights =
        model.train(inputs, targets, initialWeights, 60, 0.15);
    const auto finish = std::chrono::steady_clock::now();

    std::cout << "QNN regression example\n"
              << std::fixed << std::setprecision(6)
              << "Initial mean squared error: " << initialLoss << '\n'
              << "Final mean squared error: "
              << model.loss(inputs, trainedWeights, targets)
              << "\nPredictions after training\n";
    for (std::size_t index = 0; index < inputs.size(); ++index) {
        std::cout << "Input " << index
                  << ": prediction = " << model.forward(inputs[index], trainedWeights)
                  << ", target = " << targets[index] << '\n';
    }
    std::cout << "Training duration: "
              << std::chrono::duration<double, std::milli>(finish - start).count()
              << " ms\n";
    return 0;
}
