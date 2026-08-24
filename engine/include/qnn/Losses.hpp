#pragma once

#include <vector>

namespace QNN
{
double meanSquaredError(
    const std::vector<double>& predictions,
    const std::vector<double>& targets);

double binaryCrossEntropy(double prediction, double target);
}