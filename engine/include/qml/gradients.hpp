#pragma once

#include <functional>
#include <vector>

namespace quantum::ml {

// Calculates gradients using numerical finite differences
std::vector<double> finiteDifferenceGradient(
    const std::function<double(const std::vector<double>&)>& targetFunction,
    const std::vector<double>& parameters,
    double coordinateStep = 1e-5);

// Calculates gradients for parameters used once in standard rotation gates
std::vector<double> parameterShiftGradient(
    const std::function<double(const std::vector<double>&)>& quantumModelFunction,
    const std::vector<double>& parameters,
    double analyticShift = 1.5707963267948966);

} // namespace quantum::ml
