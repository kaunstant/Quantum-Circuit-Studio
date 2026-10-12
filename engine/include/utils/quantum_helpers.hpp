#pragma once

#include "operators.hpp"
#include "math_helpers.hpp"

#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

namespace quantum::utils {

// Calculates the normalized marginal probability of every outcome in a register
inline std::vector<double> calculateRegisterProbabilities(
    const std::vector<std::complex<double>>& globalStateVector,
    const register_view& reg)
{
    const std::size_t qubitCount = reg.context().numQubits();
    if (qubitCount >= std::numeric_limits<std::size_t>::digits ||
        reg.size() == 0 ||
        reg.size() >= std::numeric_limits<std::size_t>::digits) {
        throw std::invalid_argument("Register width is invalid for state-vector indexing");
    }
    // Confirms that the input vector represents the complete circuit state.
    const std::size_t expectedStateSize = std::size_t{1} << qubitCount;
    if (globalStateVector.size() != expectedStateSize) {
        throw std::invalid_argument("State vector size does not match the register's circuit");
    }

    // Accumulates global basis-state probabilities by their register-local outcome.
    const std::size_t outcomeCount = std::size_t{1} << reg.size();
    std::vector<double> probabilities(outcomeCount, 0.0);
    for (std::size_t basis = 0; basis < globalStateVector.size(); ++basis) {
        std::size_t registerOutcome = 0;
        for (std::size_t bit = 0; bit < reg.size(); ++bit) {
            const std::size_t globalBit = reg.startIndex() + bit;
            if ((basis & (std::size_t{1} << globalBit)) != 0) {
                registerOutcome |= std::size_t{1} << bit;
            }
        }
        const double probability = std::norm(globalStateVector[basis]);
        if (!std::isfinite(probability)) {
            throw std::invalid_argument("State vector contains a non-finite probability");
        }
        probabilities[registerOutcome] += probability;
    }

    // Normalize the marginal so callers can consume it as a probability distribution.
    double totalProbability = 0.0;
    for (const double probability : probabilities) {
        totalProbability += probability;
    }
    if (!std::isfinite(totalProbability) || totalProbability <= 0.0) {
        throw std::invalid_argument("State vector has zero or invalid norm");
    }
    for (double& probability : probabilities) {
        probability /= totalProbability;
    }
    return probabilities;
}

// Samples a phase distribution and searches its continued-fraction convergents for a period
inline std::uint64_t deducePeriodFromPhaseDistribution(
    const std::vector<double>& phaseProbabilities,
    std::uint64_t base,
    std::uint64_t modulus,
    std::size_t maxAttempts = 10000)
{
    if (phaseProbabilities.empty() ||
        phaseProbabilities.size() > std::numeric_limits<std::uint64_t>::max() ||
        maxAttempts == 0) {
        throw std::invalid_argument("Phase distribution and attempt limit must be nonempty");
    }
    if (modulus <= 1 || base == 0 || gcd(base, modulus) != 1) {
        throw std::invalid_argument("Period recovery requires modulus > 1 and a coprime base");
    }

    double probabilityTotal = 0.0;
    for (const double probability : phaseProbabilities) {
        if (!std::isfinite(probability) || probability < 0.0) {
            throw std::invalid_argument("Phase probabilities must be finite and nonnegative");
        }
        probabilityTotal += probability;
    }
    if (!std::isfinite(probabilityTotal) || probabilityTotal <= 0.0) {
        throw std::invalid_argument("Phase probability distribution has zero or invalid norm");
    }

    // Draw candidate phase outcomes according to their measured probabilities.
    std::mt19937_64 generator(std::random_device{}());
    std::discrete_distribution<std::size_t> samplePhase(
        phaseProbabilities.begin(), phaseProbabilities.end());
    const std::uint64_t phaseDenominator =
        static_cast<std::uint64_t>(phaseProbabilities.size());

    for (std::size_t attempt = 0; attempt < maxAttempts; ++attempt) {
        const std::uint64_t phaseNumerator =
            static_cast<std::uint64_t>(samplePhase(generator));
        if (phaseNumerator == 0) {
            continue;
        }
        // A convergent denominator is accepted only when it reproduces the modular period.
        for (const Rational& candidate :
             continuedFractionConvergents(phaseNumerator, phaseDenominator)) {
            if (candidate.denominator > 1 &&
                candidate.denominator < modulus &&
                modularPower(base, candidate.denominator, modulus) == 1) {
                return candidate.denominator;
            }
        }
    }
    return 0;
}

// Appends the Hadamard, controlled-power, and inverse-QFT stages of phase estimation
template <typename OracleBuilder>
void unrollQuantumPhaseEstimation(
    operators& timeline,
    const register_view& phaseRegister,
    const register_view& targetRegister,
    OracleBuilder&& oracleBuilder)
{
    if (&phaseRegister.context() != &targetRegister.context()) {
        throw std::invalid_argument("Phase and target registers must belong to the same circuit");
    }
    if (phaseRegister.size() == 0 || targetRegister.size() == 0) {
        throw std::invalid_argument("Phase estimation requires non-empty registers");
    }
    const std::size_t phaseEnd = phaseRegister.startIndex() + phaseRegister.size();
    const std::size_t targetEnd = targetRegister.startIndex() + targetRegister.size();
    if (phaseRegister.startIndex() < targetEnd && targetRegister.startIndex() < phaseEnd) {
        throw std::invalid_argument("Phase and target registers must be distinct");
    }

    // Each successive phase wire controls the next squared power of the supplied unitary.
    std::size_t power = 1;
    for (std::size_t bit = 0; bit < phaseRegister.size(); ++bit) {
        const qubit_view control = phaseRegister[bit];
        timeline.h(control);
        oracleBuilder(timeline, control, targetRegister, power);

        if (bit + 1 < phaseRegister.size()) {
            if (power > std::numeric_limits<std::size_t>::max() / 2) {
                throw std::overflow_error("Phase-estimation controlled power exceeds size_t");
            }
            power *= 2;
        }
    }
    timeline.iqft(phaseRegister);
}

} // namespace quantum::utils
