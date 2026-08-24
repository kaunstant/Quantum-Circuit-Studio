#include "Gradients.hpp"

#include <stdexcept>

namespace QNN
{
std::vector<double> parameterShift(
    const std::function<double(const std::vector<double>&)>& function,
    const std::vector<double>& parameters,
    double shift)
{
    if (shift <= 0.0)
    {
        throw std::invalid_argument("Parameter-shift must use a positive shift.");
    }

    std::vector<double> gradients(parameters.size(), 0.0);

    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        std::vector<double> plus = parameters;
        std::vector<double> minus = parameters;
        plus[index] += shift;
        minus[index] -= shift;
        gradients[index] =
            0.5 * (function(plus) - function(minus));
    }

    return gradients;
}
}