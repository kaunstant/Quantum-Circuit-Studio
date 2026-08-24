#pragma once

#include <complex>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace Console
{
// =============================
// Statevector display
// =============================

inline void printState(
    const std::vector<std::complex<double>>& state,
    const std::string& title = "Statevector")
{
    std::cout << "\n" << title << "\n";
    std::cout << std::left
              << std::setw(12) << "Basis"
              << std::setw(18) << "Real"
              << std::setw(18) << "Imaginary"
              << "Probability\n";

    for (std::size_t index = 0; index < state.size(); ++index)
    {
        std::cout << std::left
                  << std::setw(12) << ("|" + std::to_string(index) + ">")
                  << std::setw(18) << std::fixed << std::setprecision(8)
                  << state[index].real()
                  << std::setw(18) << state[index].imag()
                  << std::norm(state[index])
                  << "\n";
    }
}
}
