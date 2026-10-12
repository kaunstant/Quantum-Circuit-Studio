#include "losses.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace quantum::ml {
namespace {

void validatePairs(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    if (predictions.empty()) {
        throw std::invalid_argument("Loss and metric inputs cannot be empty");
    }
    if (predictions.size() != targets.size()) {
        throw std::invalid_argument("Prediction and target counts must match");
    }
    for (std::size_t index = 0; index < predictions.size(); ++index) {
        if (!std::isfinite(predictions[index]) || !std::isfinite(targets[index])) {
            throw std::invalid_argument("Predictions and targets must be finite");
        }
    }
}

void validateProbabilitiesAndTargets(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    validatePairs(predictions, targets);
    for (std::size_t index = 0; index < predictions.size(); ++index) {
        if (predictions[index] < 0.0 || predictions[index] > 1.0 ||
            targets[index] < 0.0 || targets[index] > 1.0) {
            throw std::invalid_argument(
                "Binary cross-entropy predictions and targets must be in [0, 1]");
        }
    }
}

void validateBinaryTargets(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    validateProbabilitiesAndTargets(predictions, targets);
    for (const double target : targets) {
        if (target != 0.0 && target != 1.0) {
            throw std::invalid_argument("Classification metrics require binary targets");
        }
    }
}

void validateClassLabels(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    validatePairs(predictions, targets);
    for (const double target : targets) {
        if (std::round(target) != target) {
            throw std::invalid_argument(
                "Classification accuracy requires integer-valued class labels");
        }
    }
}

double safeRatio(double numerator, double denominator)
{
    return denominator == 0.0 ? 0.0 : numerator / denominator;
}

} // namespace

namespace loss {

// Calculates Mean Squared Error for regression targets
double meanSquaredError(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    validatePairs(predictions, targets);
    double total = 0.0;
    for (std::size_t index = 0; index < predictions.size(); ++index) {
        const double difference = predictions[index] - targets[index];
        total += difference * difference;
    }
    const double value = total / static_cast<double>(predictions.size());
    if (!std::isfinite(value)) {
        throw std::overflow_error("Mean squared error is non-finite");
    }
    return value;
}

// Calculates Binary Cross Entropy for probability predictions
double binaryCrossEntropy(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    validateProbabilitiesAndTargets(predictions, targets);
    constexpr double epsilon = 1e-12;
    double total = 0.0;
    for (std::size_t index = 0; index < predictions.size(); ++index) {
        const double probability =
            std::clamp(predictions[index], epsilon, 1.0 - epsilon);
        total -= targets[index] * std::log(probability) +
            (1.0 - targets[index]) * std::log(1.0 - probability);
    }
    const double value = total / static_cast<double>(predictions.size());
    if (!std::isfinite(value)) {
        throw std::overflow_error("Binary cross-entropy is non-finite");
    }
    return value;
}

} // namespace loss

namespace metric {

// Calculates Root Mean Squared Error
double rootMeanSquaredError(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    return std::sqrt(loss::meanSquaredError(predictions, targets));
}

// Calculates the R-squared score
double rSquared(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    validatePairs(predictions, targets);
    double targetMean = 0.0;
    for (const double target : targets) {
        targetMean += target;
    }
    targetMean /= static_cast<double>(targets.size());

    double residualSum = 0.0;
    double totalSum = 0.0;
    for (std::size_t index = 0; index < targets.size(); ++index) {
        const double residual = targets[index] - predictions[index];
        const double centered = targets[index] - targetMean;
        residualSum += residual * residual;
        totalSum += centered * centered;
    }
    if (totalSum == 0.0) {
        throw std::domain_error("R-squared is undefined when targets have zero variance");
    }
    const double score = 1.0 - residualSum / totalSum;
    if (!std::isfinite(score)) {
        throw std::overflow_error("R-squared score is non-finite");
    }
    return score;
}

// Calculates classification accuracy by rounding predictions to the nearest class
double accuracy(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    validateClassLabels(predictions, targets);
    std::size_t correct = 0;
    for (std::size_t index = 0; index < predictions.size(); ++index) {
        correct += std::round(predictions[index]) == targets[index] ? 1U : 0U;
    }
    return static_cast<double>(correct) / static_cast<double>(predictions.size());
}

// Calculates the precision of positive predictions
double precision(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    validateBinaryTargets(predictions, targets);
    double truePositive = 0.0;
    double falsePositive = 0.0;
    for (std::size_t index = 0; index < predictions.size(); ++index) {
        if (predictions[index] >= 0.5) {
            if (targets[index] == 1.0) {
                truePositive += 1.0;
            } else {
                falsePositive += 1.0;
            }
        }
    }
    return safeRatio(truePositive, truePositive + falsePositive);
}

// Calculates the recall of positive targets
double recall(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    validateBinaryTargets(predictions, targets);
    double truePositive = 0.0;
    double falseNegative = 0.0;
    for (std::size_t index = 0; index < predictions.size(); ++index) {
        if (targets[index] == 1.0) {
            if (predictions[index] >= 0.5) {
                truePositive += 1.0;
            } else {
                falseNegative += 1.0;
            }
        }
    }
    return safeRatio(truePositive, truePositive + falseNegative);
}

// Calculates the F1 score
double f1Score(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    const double precisionValue = precision(predictions, targets);
    const double recallValue = recall(predictions, targets);
    return safeRatio(
        2.0 * precisionValue * recallValue,
        precisionValue + recallValue);
}

} // namespace metric
} // namespace quantum::ml
