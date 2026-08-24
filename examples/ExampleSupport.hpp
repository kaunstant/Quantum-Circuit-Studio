#pragma once

#include "QuantumSimulator.hpp"
#include "StatePrinter.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

inline void checkNormalized(const SimulationResult& result)
{
    double total = 0.0;

    for (const auto& amplitude : result.state)
    {
        total += std::norm(amplitude);
    }

    assert(std::abs(total - 1.0) < 1e-10);
}

inline void printExampleResult(const char* name, const SimulationResult& result)
{
    checkNormalized(result);
    Console::printState(result.state, name);
}