#include "Losses.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace QNN
{
double meanSquaredError(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    if (predictions.empty() || predictions.size() != targets.size())
    {
        throw std::invalid_argument(
            "MSE requires equally sized, non-empty vectors.");
    }

    double result = 0.0;
    for (std::size_t index = 0; index < predictions.size(); ++index)
    {
        const double difference = predictions[index] - targets[index];
        result += difference * difference;
    }

    return result / static_cast<double>(predictions.size());
}

double binaryCrossEntropy(double prediction, double target)
{
    constexpr double epsilon = 1e-12;
    const double clipped = std::clamp(prediction, epsilon, 1.0 - epsilon);
    return -(target * std::log(clipped) +
             (1.0 - target) * std::log(1.0 - clipped));
}
}