#pragma once

#include <cstddef>
#include <complex>

namespace Config
{
    // =========================
    // Numeric configuration
    // =========================

    using Real = double;
    using Complex = std::complex<Real>;


    // =========================
    // Engine limits
    // =========================

    constexpr std::size_t DEFAULT_MAX_QUBITS = 32;
    constexpr std::size_t DEFAULT_MAX_CLASSICAL_BITS = 32;


    // =========================
    // Numerical tolerance
    // =========================

    constexpr Real EPSILON = 1e-12;


    // =========================
    // Register configuration
    // =========================

    constexpr const char* DEFAULT_QUANTUM_REGISTER_NAME = "q";
    constexpr const char* DEFAULT_CLASSICAL_REGISTER_NAME = "c";

    using RegisterID = std::size_t;
    constexpr RegisterID INVALID_REGISTER_ID = static_cast<RegisterID>(-1);    
}