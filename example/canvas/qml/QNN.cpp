#include "circuit_canvas.hpp"
#include "qnn.hpp"

#include <QApplication>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    quantum::variational::ansatz variationalAnsatz(
        3,
        4,
        [](quantum::operators& operations, const std::vector<double>& weights) {
            operations.ry(0, weights.at(0));
            operations.ry(1, weights.at(1));
            operations.ry(2, weights.at(2));
            operations.cx(1, 0);
            operations.rz(0, weights.at(3));
        });

    quantum::ml::qnn model(
        3,
        [](quantum::operators& operations, const std::vector<double>& features) {
            operations.rx(0, features.at(0));
            operations.rx(1, features.at(1));
            operations.rx(2, features.at(2));
        },
        variationalAnsatz);
    model.setOutputMode(quantum::ml::OutputMode::Probability);
    model.setOutputQubit(0);

    const std::vector<std::vector<double>> trainingFeatures = {
        {0.92, 1.0, 0.0},
        {0.65, 0.0, 1.0},
        {0.85, 1.0, 1.0},
        {0.58, 0.0, 0.0}
    };
    const std::vector<double> expectedTargets = {1.0, 0.0, 1.0, 0.0};
    const std::vector<double> initialWeights = {0.1, -0.2, 0.5, 0.15};
    const double initialLoss =
        model.loss(trainingFeatures, initialWeights, expectedTargets);

    const auto trainingStart = std::chrono::steady_clock::now();
    const std::vector<double> trainedWeights =
        model.train(trainingFeatures, expectedTargets, initialWeights, 100, 0.08);
    const auto trainingFinish = std::chrono::steady_clock::now();
    const double finalLoss =
        model.loss(trainingFeatures, trainedWeights, expectedTargets);
    const std::vector<double> predictions =
        model.forwardBatch(trainingFeatures, trainedWeights);

    std::cout << "QNN placement prediction example\n"
              << std::fixed << std::setprecision(5)
              << "  Initial binary cross-entropy: " << initialLoss << '\n'
              << "  Final binary cross-entropy: " << finalLoss << '\n'
              << "  Predictions\n";
    for (std::size_t index = 0; index < trainingFeatures.size(); ++index) {
        std::cout << "    CGPA " << trainingFeatures[index][0] * 10.0
                  << ", major " << trainingFeatures[index][1]
                  << ", minor " << trainingFeatures[index][2]
                  << " -> placement probability " << predictions[index]
                  << " (label " << expectedTargets[index] << ")\n";
    }
    std::cout << "Training/simulation duration: "
              << std::chrono::duration<double, std::milli>(
                     trainingFinish - trainingStart).count()
              << " ms\n"
              << std::flush;

    auto snapshot =
        model.getAssembledCircuit(trainingFeatures.front(), trainedWeights);
    app::circuit_canvas canvas(
        snapshot.layout,
        snapshot.operations,
        QStringLiteral("Trained QNN Placement Predictor"));
    canvas.setWindowTitle(QStringLiteral("Trained QNN Circuit"));
    canvas.resize(1300, 560);
    canvas.draw();
    canvas.show();
    return application.exec();
}
