#include "losses.hpp"
#include "qnn.hpp"
#include "qml_helpers.hpp"
#include "csv_helpers.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

constexpr std::size_t featureCount = 11;
constexpr std::size_t targetColumn = featureCount;
constexpr std::size_t qubitCount = 6;
constexpr std::size_t ansatzLayers = 3;
constexpr std::size_t externalEpochCount = 1;
constexpr std::size_t batchSize = 32;
constexpr std::size_t maximumRowsToPrint = 5;
constexpr double learningRate = 0.02;

quantum::ml::qnn makeVineQualityModel()
{
    const std::size_t totalWeights = qubitCount * 2 * ansatzLayers;
    quantum::variational::ansatz ansatz(
        qubitCount,
        totalWeights,
        [](quantum::operators& operations, const std::vector<double>& weights) {
            for (std::size_t layer = 0; layer < ansatzLayers; ++layer) {
                const std::size_t layerOffset = layer * qubitCount * 2;
                for (std::size_t wire = 0; wire < qubitCount; ++wire) {
                    operations.ry(
                        static_cast<int>(wire),
                        weights.at(layerOffset + wire * 2));
                    operations.rz(
                        static_cast<int>(wire),
                        weights.at(layerOffset + wire * 2 + 1));
                }
                for (std::size_t wire = 0; wire + 1 < qubitCount; ++wire) {
                    operations.cx(
                        static_cast<int>(wire + 1),
                        static_cast<int>(wire));
                }
                operations.cx(0, static_cast<int>(qubitCount - 1));
            }
        });

    return quantum::ml::qnn(
        qubitCount,
        [](quantum::operators& operations, const std::vector<double>& features) {
            if (features.size() != featureCount) {
                throw std::invalid_argument(
                    "Vine quality model expects 11 input features");
            }
            for (std::size_t index = 0; index < features.size(); ++index) {
                const int wire = static_cast<int>(index % qubitCount);
                if (index < qubitCount) {
                    operations.rx(wire, features[index]);
                } else {
                    operations.ry(wire, features[index]);
                }
            }
        },
        ansatz);
}

} // namespace

int main()
{
    try {
        const std::filesystem::path sourcePath =
            std::filesystem::path("example") /
            "Vine Quality Predictor" /
            "dataset" /
            "data.csv";
        const auto sourceRows =
            quantum::utils::loadCSV(sourcePath.string(), true);
        const auto split = quantum::utils::splitAndSaveDataset(
            sourceRows, 1, 5, 42, sourcePath.string(), true);
        const auto trainingRows =
            quantum::utils::loadCSV(split.trainPath.string(), true);
        const auto testRows =
            quantum::utils::loadCSV(split.testPath.string(), true);

        const auto scaler =
            quantum::ml::fitRegressionScaler(trainingRows, targetColumn);
        const auto trainingData =
            quantum::ml::transformRegressionDataset(trainingRows, scaler);
        const auto testData =
            quantum::ml::transformRegressionDataset(testRows, scaler);

        auto model = makeVineQualityModel();
        const std::vector<double> initialWeights(model.getWeightsCount(), 0.05);
        const auto trainingStart = std::chrono::steady_clock::now();
        model.train(
            trainingData.features,
            trainingData.encodedTargets,
            initialWeights,
            externalEpochCount,
            batchSize,
            learningRate,
            quantum::ml::LossType::MeanSquaredError);
        const auto trainingFinish = std::chrono::steady_clock::now();
        const double trainingMilliseconds =
            std::chrono::duration<double, std::milli>(
                trainingFinish - trainingStart).count();

        model.save("vine");
        const auto predictions = quantum::ml::inverseTransformTargets(
            model.forwardBatch(testData.features), scaler);
        std::cout << std::fixed << std::setprecision(4)
                  << "Vine quality QNN predictor\n"
                  << "  Training rows: " << trainingData.targets.size() << '\n'
                  << "  External epochs: " << externalEpochCount << '\n'
                  << "  Batch size: " << batchSize << '\n'
                  << "  Test rows: " << testData.targets.size() << '\n'
                  << "  Training time: " << trainingMilliseconds << " ms\n"
                  << "  Test RMSE: "
                  << quantum::ml::metric::rootMeanSquaredError(
                         predictions, testData.targets)
                  << '\n'
                  << "  Test rounded-class accuracy: "
                  << 100.0 * quantum::ml::metric::accuracy(
                         predictions, testData.targets)
                  << "%\n"
                  << "  Test R-squared: "
                  << quantum::ml::metric::rSquared(predictions, testData.targets)
                  << "\n  Saved model: outputs/vine.qnn\n\n";

        const std::size_t rowsToPrint =
            std::min(maximumRowsToPrint, testData.targets.size());
        for (std::size_t row = 0; row < rowsToPrint; ++row) {
            std::cout << "Input features: [";
            for (std::size_t feature = 0;
                 feature < testData.rawFeatures[row].size();
                 ++feature) {
                if (feature != 0) {
                    std::cout << ", ";
                }
                std::cout << testData.rawFeatures[row][feature];
            }
            std::cout << "]\n"
                      << "  Predicted quality: " << predictions[row]
                      << " (rounded " << std::round(predictions[row]) << ")\n"
                      << "  Actual quality: " << testData.targets[row] << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << "Vine quality prediction failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
