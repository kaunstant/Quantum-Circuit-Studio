#include "losses.hpp"
#include "qnn.hpp"
#include "qml_helpers.hpp"
#include "utils/csv_helpers.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

constexpr std::size_t featureCount = 11;
constexpr std::size_t qubitCount = 3;
constexpr std::size_t targetColumn = featureCount;
constexpr std::size_t externalEpochCount = 1;
constexpr std::size_t batchSize = 4;

} // namespace

int main()
{
    const auto wallStart = std::chrono::steady_clock::now();
    const std::filesystem::path sourcePath =
        std::filesystem::path("example") /
        "Vine Quality Predictor" /
        "dataset" /
        "data.csv";
    const auto allRows = quantum::utils::loadCSV(sourcePath.string(), true);
    if (allRows.size() < 12) {
        throw std::runtime_error("Classification benchmark requires at least 12 rows");
    }
    std::vector<std::vector<double>> rows;
    rows.reserve(12);
    for (std::size_t sample = 0; sample < 12; ++sample) {
        const std::size_t index =
            sample * (allRows.size() - 1) / 11;
        rows.push_back(allRows[index]);
    }
    const std::vector<std::vector<double>> trainingRows(rows.begin(), rows.begin() + 8);
    const std::vector<std::vector<double>> testRows(rows.begin() + 8, rows.end());
    const auto scaler =
        quantum::ml::fitRegressionScaler(trainingRows, targetColumn);
    const auto trainingData =
        quantum::ml::transformRegressionDataset(trainingRows, scaler);
    const auto testData =
        quantum::ml::transformRegressionDataset(testRows, scaler);
    std::vector<double> trainingTargets;
    std::vector<double> testTargets;
    for (const double quality : trainingData.targets) {
        trainingTargets.push_back(quality >= 6.0 ? 1.0 : 0.0);
    }
    for (const double quality : testData.targets) {
        testTargets.push_back(quality >= 6.0 ? 1.0 : 0.0);
    }

    quantum::variational::ansatz ansatz(
        qubitCount,
        qubitCount * 2,
        [](quantum::operators& operations, const std::vector<double>& weights) {
            for (std::size_t wire = 0; wire < qubitCount; ++wire) {
                operations.ry(static_cast<int>(wire), weights[wire * 2]);
                operations.rz(static_cast<int>(wire), weights[wire * 2 + 1]);
            }
            operations.cx(1, 0);
            operations.cx(2, 1);
        });
    quantum::ml::qnn model(
        qubitCount,
        [](quantum::operators& operations, const std::vector<double>& features) {
            if (features.size() != featureCount) {
                throw std::invalid_argument("Classification input must have 11 features");
            }
            for (std::size_t index = 0; index < features.size(); ++index) {
                const int wire = static_cast<int>(index % qubitCount);
                switch ((index / qubitCount) % 3) {
                case 0: operations.rx(wire, features[index]); break;
                case 1: operations.ry(wire, features[index]); break;
                default: operations.rz(wire, features[index]); break;
                }
            }
        },
        ansatz);
    model.setOutputMode(quantum::ml::OutputMode::Probability);
    model.setOutputQubit(0);
    model.train(
        trainingData.features,
        trainingTargets,
        std::vector<double>(model.getWeightsCount(), 0.05),
        externalEpochCount,
        batchSize,
        0.02,
        quantum::ml::LossType::BinaryCrossEntropy);
    const auto predictions = model.forwardBatch(testData.features);
    const double accuracy = quantum::ml::metric::accuracy(predictions, testTargets);
    const auto wallFinish = std::chrono::steady_clock::now();

    std::cout << "workload=QNN classification, train_rows=8, test_rows=4, "
                 "external_epochs=1, batch_size=4\n"
              << "accuracy=" << accuracy << '\n'
              << "end_to_end_wall_time_ms="
              << std::chrono::duration<double, std::milli>(
                     wallFinish - wallStart).count()
              << '\n';
}
