#include "circuit_canvas.hpp"
#include "qnn.hpp"

#include <QApplication>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    const std::vector<std::vector<double>> trainingFeatures = {
        {0.92, 1.0, 0.0},
        {0.65, 0.0, 1.0},
        {0.85, 1.0, 1.0},
        {0.58, 0.0, 0.0}
    };
    const std::vector<double> expectedTargets = {1.0, 0.0, 1.0, 0.0};
    const std::vector<double> initialWeights = {0.1, -0.2, 0.5, 0.15, -0.1, 0.25};
    std::vector<double> predictionsBeforeSave;

    {
        quantum::ml::qnn model(3, 1);
        model.setOutputMode(quantum::ml::OutputMode::Probability);
        model.setOutputQubit(0);

        const double initialLoss =
            model.loss(trainingFeatures, initialWeights, expectedTargets);
        const auto trainingStart = std::chrono::steady_clock::now();
        model.train(trainingFeatures, expectedTargets, initialWeights, 30, 0.08);
        const auto trainingFinish = std::chrono::steady_clock::now();
        predictionsBeforeSave = model.forwardBatch(trainingFeatures);

        std::cout << "QNN model persistence round-trip\n"
                  << std::fixed << std::setprecision(6)
                  << "  Qubits: " << model.getNumQubits() << '\n'
                  << "  Trainable weights: " << model.getWeightsCount() << '\n'
                  << "  Initial binary cross-entropy: " << initialLoss << '\n'
                  << "  Final binary cross-entropy: " << model.getTrainingLoss() << '\n'
                  << "  Training duration: "
                  << std::chrono::duration<double, std::milli>(
                         trainingFinish - trainingStart).count()
                  << " ms\n";
        model.save("placement_classifier");
        std::cout << "  Saved trained model to outputs/placement_classifier.qnn\n\n";
    }

    quantum::ml::qnn restoredModel;
    restoredModel.load("outputs/placement_classifier.qnn");
    const std::vector<double> predictionsAfterLoad =
        restoredModel.forwardBatch(trainingFeatures);

    std::cout << "Reloaded model state\n"
              << "  Qubits: " << restoredModel.getNumQubits() << '\n'
              << "  Ansatz layers: " << restoredModel.getLayersCount() << '\n'
              << "  Trainable weights: " << restoredModel.getWeightsCount() << '\n'
              << "  Stored weights: " << restoredModel.getWeights().size() << '\n'
              << "  Output mode: "
              << (restoredModel.getOutputMode() == quantum::ml::OutputMode::Probability
                      ? "probability"
                      : "expectation")
              << " on wire " << restoredModel.getOutputQubit() << '\n'
              << "  Trained: " << std::boolalpha << restoredModel.isTrained() << '\n'
              << "  Stored final loss: " << restoredModel.getTrainingLoss() << '\n'
              << "  Predictions (before save -> after load)\n"
              << std::flush;

    for (std::size_t index = 0; index < predictionsAfterLoad.size(); ++index) {
        const double difference =
            std::abs(predictionsBeforeSave.at(index) - predictionsAfterLoad[index]);
        if (difference > 1e-12) {
            throw std::runtime_error("QNN predictions changed after save/load");
        }
        std::cout << "    Sample " << index + 1 << ": "
                  << predictionsBeforeSave[index] << " -> "
                  << predictionsAfterLoad[index]
                  << " (target " << expectedTargets[index] << ")\n"
                  << std::flush;
    }

    auto snapshot = restoredModel.getAssembledCircuit(trainingFeatures.front());
    app::circuit_canvas canvas(
        snapshot.layout,
        snapshot.operations,
        QStringLiteral("Reloaded QNN Placement Predictor"));
    canvas.setWindowTitle(QStringLiteral("Reloaded QNN Circuit"));
    canvas.resize(1300, 560);
    canvas.draw();
    canvas.show();
    return application.exec();
}
