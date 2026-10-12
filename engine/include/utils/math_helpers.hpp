#pragma once

#include "operators.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace quantum::utils {

struct Rational {
    std::uint64_t numerator;
    std::uint64_t denominator;
};

// Greatest Common Divisor
inline std::uint64_t gcd(std::uint64_t lhs, std::uint64_t rhs) noexcept
{
    while (rhs != 0) {
        const std::uint64_t remainder = lhs % rhs;
        lhs = rhs;
        rhs = remainder;
    }
    return lhs;
}

inline std::uint64_t modularMultiply(
    std::uint64_t lhs,
    std::uint64_t rhs,
    std::uint64_t modulus)
{
    if (modulus == 0) {
        throw std::invalid_argument("Modular multiplication modulus must be nonzero");
    }
    if (modulus == 1) {
        return 0;
    }

    const auto addModulo = [modulus](std::uint64_t left, std::uint64_t right) {
        return left >= modulus - right ? left - (modulus - right) : left + right;
    };
    lhs %= modulus;
    rhs %= modulus;
    std::uint64_t product = 0;
    while (rhs != 0) {
        if ((rhs & 1U) != 0) {
            product = addModulo(product, lhs);
        }
        rhs >>= 1U;
        if (rhs != 0) {
            lhs = addModulo(lhs, lhs);
        }
    }
    return product;
}

inline std::uint64_t modularPower(
    std::uint64_t base,
    std::uint64_t exponent,
    std::uint64_t modulus)
{
    if (modulus == 0) {
        throw std::invalid_argument("Modular exponentiation modulus must be nonzero");
    }
    if (modulus == 1) {
        return 0;
    }
    base %= modulus;
    std::uint64_t result = 1;
    while (exponent != 0) {
        if ((exponent & 1U) != 0) {
            result = modularMultiply(result, base, modulus);
        }
        exponent >>= 1U;
        if (exponent != 0) {
            base = modularMultiply(base, base, modulus);
        }
    }
    return result;
}

// Construct the permutation matrix for |x> -> |multiplier*x mod modulus>
// below modulus, leaving the remaining basis states unchanged.
inline quantum::Matrix2D modularMultiplicationMatrix(
    std::size_t width,
    std::uint64_t multiplier,
    std::uint64_t modulus)
{
    if (modulus == 0) {
        throw std::invalid_argument("Modular multiplication modulus must be nonzero");
    }
    if (width == 0 || width >= std::numeric_limits<std::size_t>::digits) {
        throw std::invalid_argument("Register width must be valid for matrix basis indexing");
    }
    const std::size_t dimension = std::size_t{1} << width;
    if (modulus > dimension) {
        throw std::invalid_argument("Modulus does not fit in the selected register");
    }
    multiplier %= modulus;
    if (gcd(multiplier, modulus) != 1) {
        throw std::invalid_argument("Modular multiplier must be coprime to the modulus");
    }

    quantum::Matrix2D matrix(
        dimension,
        std::vector<quantum::Complex>(dimension, quantum::Complex{0.0, 0.0}));
    for (std::size_t value = 0; value < dimension; ++value) {
        const std::size_t mapped = value < modulus
            ? static_cast<std::size_t>(modularMultiply(multiplier, value, modulus))
            : value;
        matrix[mapped][value] = quantum::Complex{1.0, 0.0};
    }
    return matrix;
}

// Return convergents in order of increasing continued-fraction depth.
inline std::vector<Rational> continuedFractionConvergents(
    std::uint64_t numerator,
    std::uint64_t denominator)
{
    if (denominator == 0) {
        throw std::invalid_argument("Continued-fraction denominator must be nonzero");
    }

    const auto checkedMultiplyAdd = [](std::uint64_t multiplier,
                                       std::uint64_t value,
                                       std::uint64_t addend) {
        if (value != 0 &&
            multiplier > (std::numeric_limits<std::uint64_t>::max() - addend) / value) {
            throw std::overflow_error("Continued-fraction convergent exceeds uint64_t");
        }
        return multiplier * value + addend;
    };

    std::vector<Rational> convergents;
    std::uint64_t previousNumerator = 0;
    std::uint64_t currentNumerator = 1;
    std::uint64_t previousDenominator = 1;
    std::uint64_t currentDenominator = 0;

    while (denominator != 0) {
        const std::uint64_t coefficient = numerator / denominator;
        const std::uint64_t remainder = numerator % denominator;
        const std::uint64_t nextNumerator =
            checkedMultiplyAdd(coefficient, currentNumerator, previousNumerator);
        const std::uint64_t nextDenominator =
            checkedMultiplyAdd(coefficient, currentDenominator, previousDenominator);
        convergents.push_back({nextNumerator, nextDenominator});

        previousNumerator = currentNumerator;
        currentNumerator = nextNumerator;
        previousDenominator = currentDenominator;
        currentDenominator = nextDenominator;
        numerator = denominator;
        denominator = remainder;
    }

    return convergents;
}

} // namespace quantum::utils
