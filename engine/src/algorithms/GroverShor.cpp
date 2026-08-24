#include "GroverShor.hpp"

#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace Algorithms
{
GroverOracle::GroverOracle(int qubitCount, int markedState) : qubitCount_(qubitCount), markedState_(markedState)
{
    if (qubitCount <= 0 || qubitCount > 20 || markedState < 0 || markedState >= (1 << qubitCount))
        throw std::invalid_argument("Grover oracle state is outside its search space.");
}

void GroverOracle::appendTo(QuantumCircuit& circuit) const
{
    for (int qubit = 0; qubit < qubitCount_; ++qubit)
        if ((markedState_ & (1 << qubit)) == 0) circuit.x(qubit);
    std::vector<int> controls; for (int qubit = 0; qubit < qubitCount_ - 1; ++qubit) controls.push_back(qubit);
    circuit.h(qubitCount_ - 1); circuit.multiControlledX(controls, qubitCount_ - 1); circuit.h(qubitCount_ - 1);
    for (int qubit = 0; qubit < qubitCount_; ++qubit)
        if ((markedState_ & (1 << qubit)) == 0) circuit.x(qubit);
}

int GroverOracle::qubitCount() const { return qubitCount_; }
int GroverOracle::markedState() const { return markedState_; }

GroverOperator::GroverOperator(const GroverOracle& oracle) : oracle_(oracle) {}

void GroverOperator::appendTo(QuantumCircuit& circuit) const
{
    oracle_.appendTo(circuit); for (int qubit = 0; qubit < oracle_.qubitCount(); ++qubit) circuit.h(qubit); for (int qubit = 0; qubit < oracle_.qubitCount(); ++qubit) circuit.x(qubit);
    std::vector<int> controls; for (int qubit = 0; qubit < oracle_.qubitCount() - 1; ++qubit) controls.push_back(qubit);
    circuit.h(oracle_.qubitCount() - 1); circuit.multiControlledX(controls, oracle_.qubitCount() - 1); circuit.h(oracle_.qubitCount() - 1);
    for (int qubit = 0; qubit < oracle_.qubitCount(); ++qubit) circuit.x(qubit); for (int qubit = 0; qubit < oracle_.qubitCount(); ++qubit) circuit.h(qubit);
}

GroverSearch::GroverSearch(GroverOracle oracle) : oracle_(std::move(oracle)) {}

int GroverSearch::iterations() const { return std::max(1, static_cast<int>(std::floor(std::acos(-1.0) * std::sqrt(static_cast<double>(1 << oracle_.qubitCount())) / 4.0))); }

QuantumCircuit GroverSearch::circuit() const
{
    QuantumCircuit circuit(oracle_.qubitCount()); for (int qubit = 0; qubit < oracle_.qubitCount(); ++qubit) circuit.h(qubit);
    GroverOperator operation(oracle_); for (int iteration = 0; iteration < iterations(); ++iteration) operation.appendTo(circuit); return circuit;
}

SimulationResult GroverSearch::run() const { return QuantumSimulator().run(circuit()); }

Shor::Shor(long long number) : number_(number)
{
    if (number < 2) throw std::invalid_argument("Shor requires an integer greater than one.");
}

ShorFactors Shor::factor() const { return shorFactor(number_); }

QuantumCircuit Shor::periodFindingCircuit(int countingQubits) const
{
    long long base = 2; while (base < number_ && gcd(base, number_) != 1) ++base;
    if (base >= number_) throw std::invalid_argument("No coprime base is available for this number.");
    return shorPeriodFindingCircuit(base, number_, countingQubits);
}

long long gcd(long long left, long long right) { while (right != 0) { long long remainder = left % right; left = right; right = remainder; } return left < 0 ? -left : left; }
long long modularMultiply(long long left, long long right, long long modulus)
{
    long long result = 0; left %= modulus;
    while (right > 0) { if (right & 1) result = (result >= modulus - left) ? result - (modulus - left) : result + left; left = (left >= modulus - left) ? left - (modulus - left) : left + left; right >>= 1; }
    return result;
}
long long modularPower(long long base, long long exponent, long long modulus) { long long result = 1 % modulus; while (exponent > 0) { if (exponent & 1) result = modularMultiply(result, base, modulus); base = modularMultiply(base, base, modulus); exponent >>= 1; } return result; }

long long classicalOrder(long long base, long long modulus)
{
    long long value = 1; for (long long order = 1; order <= modulus * modulus; ++order) { value = modularMultiply(value, base, modulus); if (value == 1) return order; } return 0;
}

ShorFactors classicalFactor(long long number)
{
    if (number < 2) return {}; for (long long divisor = 2; divisor * divisor <= number; ++divisor) if (number % divisor == 0) return {divisor, number / divisor, true}; return {};
}

QuantumCircuit modularExponentiation(long long base, long long modulus, int countingQubits)
{
    if (base <= 1 || modulus <= 2 || countingQubits <= 0) throw std::invalid_argument("Invalid modular exponentiation parameters.");
    int workQubits = 0; while ((1LL << workQubits) < modulus) ++workQubits;
    QuantumCircuit circuit(countingQubits + workQubits); circuit.x(countingQubits);
    long long multiplier = base % modulus;
    for (int counting = 0; counting < countingQubits; ++counting)
    {
        const std::size_t dimension = std::size_t{1} << workQubits;
        Matrix permutation(dimension, std::vector<std::complex<double>>(dimension));
        for (long long input = 0; input < static_cast<long long>(dimension); ++input)
        {
            long long output = input < modulus ? modularMultiply(input, multiplier, modulus) : input;
            permutation[static_cast<std::size_t>(output)][static_cast<std::size_t>(input)] = 1.0;
        }
        std::vector<int> targets; for (int bit = 0; bit < workQubits; ++bit) targets.push_back(countingQubits + bit);
        circuit.controlled(permutation, counting, targets);
        multiplier = modularMultiply(multiplier, multiplier, modulus);
    }
    return circuit;
}

QuantumCircuit shorPeriodFindingCircuit(long long base, long long modulus, int countingQubits)
{
    QuantumCircuit circuit = modularExponentiation(base, modulus, countingQubits);
    const double pi = std::acos(-1.0);
    for (int offset = 0; offset < countingQubits / 2; ++offset) circuit.swap(offset, countingQubits - 1 - offset);
    for (int target = countingQubits - 1; target >= 0; --target)
    {
        for (int control = target + 1; control < countingQubits; ++control)
            circuit.controlledPhase(control, target, -pi / std::pow(2.0, control - target));
        circuit.h(target);
    }
    return circuit;
}

ShorFactors shorFactor(long long number)
{
    if (number % 2 == 0) return {2, number / 2, true}; ShorFactors direct = classicalFactor(number); if (direct.success) return direct;
    for (long long base = 2; base < number; ++base) { long long divisor = gcd(base, number); if (divisor > 1) return {divisor, number / divisor, true}; long long order = classicalOrder(base, number); if (order > 0 && order % 2 == 0) { long long plus = gcd(modularPower(base, order / 2, number) + 1, number); long long minus = gcd(modularPower(base, order / 2, number) - 1, number); if (plus > 1 && plus < number) return {plus, number / plus, true}; if (minus > 1 && minus < number) return {minus, number / minus, true}; } } return {};
}
}