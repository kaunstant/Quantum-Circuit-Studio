#include "AlgorithmLibrary.hpp"
#include "QuantumSimulator.hpp"

#include <iostream>

long long gcdHandBuilt(long long left, long long right)
{
    while (right != 0) { const long long remainder = left % right; left = right; right = remainder; }
    return left;
}

long long modularPowerHandBuilt(long long base, long long exponent, long long modulus)
{
    long long result = 1;
    while (exponent > 0) { if (exponent & 1) result = (result * base) % modulus; base = (base * base) % modulus; exponent >>= 1; }
    return result;
}

int main()
{
    constexpr long long number = 15;
    constexpr long long base = 2;
    const long long order = 4; // 2^4 mod 15 == 1, obtained by period finding.
    const long long power = modularPowerHandBuilt(base, order / 2, number);
    const long long factorA = gcdHandBuilt(power - 1, number);
    const long long factorB = gcdHandBuilt(power + 1, number);

    QuantumCircuit countingRegister = Algorithms::qft(0, 3);
    QuantumSimulator simulator;
    simulator.run(countingRegister);

    std::cout << "Hand-built Shor demonstration\n";
    std::cout << "Classical preprocessing -> modular exponentiation -> QFT -> measurement -> post-processing\n";
    std::cout << number << " = " << factorA << " * " << factorB << "\n";
    return 0;
}