#include "gradients.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace quantum::ml {
namespace {

void validateFunctionAndParameters(
    const std::function<double(const std::vector<double>&)>& function,
    const std::vector<double>& parameters)
{
    if (!function) {
        throw std::invalid_argument("Gradient calculation requires a target function");
    }
    for (const double parameter : parameters) {
        if (!std::isfinite(parameter)) {
            throw std::invalid_argument("Gradient parameters must be finite");
        }
    }
}

double evaluateFinite(
    const std::function<double(const std::vector<double>&)>& function,
    const std::vector<double>& parameters)
{
    const double value = function(parameters);
    if (!std::isfinite(value)) {
        throw std::runtime_error("Gradient target function returned a non-finite value");
    }
    return value;
}

} // namespace

// Calculates gradients using numerical finite differences
std::vector<double> finiteDifferenceGradient(
    const std::function<double(const std::vector<double>&)>& targetFunction,
    const std::vector<double>& parameters,
    double coordinateStep)
{
    validateFunctionAndParameters(targetFunction, parameters);
    if (!std::isfinite(coordinateStep) || coordinateStep <= 0.0) {
        throw std::invalid_argument("Finite-difference step must be positive and finite");
    }

    std::vector<double> gradient(parameters.size());
    for (std::size_t index = 0; index < parameters.size(); ++index) {
        std::vector<double> plus = parameters;
        std::vector<double> minus = parameters;
        plus[index] += coordinateStep;
        minus[index] -= coordinateStep;
        const double positiveValue = evaluateFinite(targetFunction, plus);
        const double negativeValue = evaluateFinite(targetFunction, minus);
        gradient[index] = (positiveValue - negativeValue) / (2.0 * coordinateStep);
        if (!std::isfinite(gradient[index])) {
            throw std::runtime_error("Finite-difference gradient is non-finite");
        }
    }
    return gradient;
}

// Calculates gradients for parameters used once in standard rotation gates
std::vector<double> parameterShiftGradient(
    const std::function<double(const std::vector<double>&)>& quantumModelFunction,
    const std::vector<double>& parameters,
    double analyticShift)
{
    validateFunctionAndParameters(quantumModelFunction, parameters);
    if (!std::isfinite(analyticShift) ||
        std::abs(std::sin(analyticShift)) <= 1e-12) {
        throw std::invalid_argument(
            "Parameter-shift angle must be finite and have a nonzero sine");
    }

    const double scale = 2.0 * std::sin(analyticShift);
    std::vector<double> gradient(parameters.size());
    for (std::size_t index = 0; index < parameters.size(); ++index) {
        std::vector<double> plus = parameters;
        std::vector<double> minus = parameters;
        plus[index] += analyticShift;
        minus[index] -= analyticShift;
        const double positiveValue = evaluateFinite(quantumModelFunction, plus);
        const double negativeValue = evaluateFinite(quantumModelFunction, minus);
        gradient[index] = (positiveValue - negativeValue) / scale;
        if (!std::isfinite(gradient[index])) {
            throw std::runtime_error("Parameter-shift gradient is non-finite");
        }
    }
    return gradient;
}

} // namespace quantum::ml
