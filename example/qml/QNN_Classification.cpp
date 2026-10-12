#include "qnn.hpp"
#include "losses.hpp"

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
    model.setOutputMode(quantum::ml::OutputMode::Probability);
    model.setOutputQubit(0);

    const std::vector<std::vector<double>> inputs = {
        {0.0},
        {std::acos(-1.0)}
    };
    const std::vector<double> targets = {0.0, 1.0};
    const std::vector<double> initialWeights = {0.35};

    const auto start = std::chrono::steady_clock::now();
    const std::vector<double> trainedWeights =
        model.train(
            inputs,
            targets,
            initialWeights,
            100,
            0.1,
            quantum::ml::LossType::Automatic);
    const auto finish = std::chrono::steady_clock::now();

    std::vector<double> predictions;
    predictions.reserve(inputs.size());
    std::cout << "QNN classification example\n"
              << "Class-one probabilities\n"
              << std::fixed << std::setprecision(6);
    for (std::size_t index = 0; index < inputs.size(); ++index) {
        predictions.push_back(model.forward(inputs[index], trainedWeights));
        std::cout << "Input " << index << ": " << predictions.back()
                  << ", target = " << targets[index] << '\n';
    }
    const auto gradient = model.backward(inputs.front(), trainedWeights);
    std::cout << "Binary cross-entropy: "
              << quantum::ml::loss::binaryCrossEntropy(predictions, targets) << '\n'
              << "Accuracy: "
              << quantum::ml::metric::accuracy(predictions, targets) << '\n'
              << "Probability gradient at input 0: " << gradient.front() << '\n'
              << "Training duration: "
              << std::chrono::duration<double, std::milli>(finish - start).count()
              << " ms\n";
    return 0;
}
