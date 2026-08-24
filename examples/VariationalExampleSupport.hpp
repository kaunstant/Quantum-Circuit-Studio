#pragma once

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "Variational.hpp"

inline void printOptimizationResult(
    const std::string& title,
    const std::vector<double>& initialParameters,
    const Variational::OptimizationResult& result,
    const std::string& valueName)
{
    std::cout << "\n" << title << "\n";
    std::cout << "Initial parameters: ";
    for (double parameter : initialParameters)
        std::cout << std::fixed << std::setprecision(6) << parameter << " ";
    std::cout << "\nFinal parameters:   ";
    for (double parameter : result.parameters)
        std::cout << std::fixed << std::setprecision(6) << parameter << " ";
    std::cout << "\n" << valueName << ": " << result.value
              << "\nIterations: " << result.iterations << "\n";
}