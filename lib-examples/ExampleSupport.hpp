#pragma once

#include <QCS>

#include <cassert>
#include <cmath>
#include <iostream>

inline void printExampleResult(const char* name, const SimulationResult& result)
{
    double total = 0.0;
    for (const auto& amplitude : result.state) total += std::norm(amplitude);
    assert(std::abs(total - 1.0) < 1e-10);

    std::cout << name << "\n";
    for (std::size_t index = 0; index < result.state.size(); ++index)
    {
        if (std::norm(result.state[index]) > 1e-12)
            std::cout << "|" << index << ">: " << result.state[index] << "\n";
    }
}