#pragma once

#include <functional>
#include <vector>

namespace QNN
{
std::vector<double> parameterShift(
    const std::function<double(const std::vector<double>&)>& function,
    const std::vector<double>& parameters,
    double shift = 1.5707963267948966);
}