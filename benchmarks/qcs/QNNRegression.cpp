#include "losses.hpp"
#include "qnn.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

int main()
{
    const auto wallStart = std::chrono::steady_clock::now();
    quantum::variational::ansatz ansatz(
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
        ansatz);

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
    constexpr std::size_t externalEpochCount = 60;
    constexpr std::size_t batchSize = 3;
    model.train(
        inputs,
        targets,
        {0.6},
        externalEpochCount,
        batchSize,
        0.15,
        quantum::ml::LossType::MeanSquaredError);
    const auto predictions = model.forwardBatch(inputs);
    const double rmse =
        quantum::ml::metric::rootMeanSquaredError(predictions, targets);
    const auto wallFinish = std::chrono::steady_clock::now();

    std::cout << "workload=QNN regression, train_rows=3, epochs=60, batch_size=3\n"
              << "RMSE=" << rmse << '\n'
              << "end_to_end_wall_time_ms="
              << std::chrono::duration<double, std::milli>(
                     wallFinish - wallStart).count()
              << '\n';
}
