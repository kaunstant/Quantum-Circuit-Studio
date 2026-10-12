#pragma once

#include <vector>

namespace quantum::ml {

namespace loss {

// Calculates Mean Squared Error for regression targets
double meanSquaredError(
    const std::vector<double>& predictions,
    const std::vector<double>& targets);

// Calculates Binary Cross Entropy for probability predictions
double binaryCrossEntropy(
    const std::vector<double>& predictions,
    const std::vector<double>& targets);

} // namespace loss

namespace metric {

// Calculates Root Mean Squared Error
double rootMeanSquaredError(
    const std::vector<double>& predictions,
    const std::vector<double>& targets);

// Calculates the R-squared score
double rSquared(
    const std::vector<double>& predictions,
    const std::vector<double>& targets);

// Calculates class accuracy by rounding predictions to the nearest integer label
double accuracy(
    const std::vector<double>& predictions,
    const std::vector<double>& targets);

// Calculates the precision of positive predictions
double precision(
    const std::vector<double>& predictions,
    const std::vector<double>& targets);

// Calculates the recall of positive targets
double recall(
    const std::vector<double>& predictions,
    const std::vector<double>& targets);

// Calculates the F1 score
double f1Score(
    const std::vector<double>& predictions,
    const std::vector<double>& targets);

} // namespace metric

} // namespace quantum::ml
