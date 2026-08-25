#include <QCS>

#include <iostream>

long long gcdHandBuilt(long long left, long long right) { while (right != 0) { const long long remainder = left % right; left = right; right = remainder; } return left; }
long long modularPowerHandBuilt(long long base, long long exponent, long long modulus) { long long result = 1; while (exponent > 0) { if (exponent & 1) result = (result * base) % modulus; base = (base * base) % modulus; exponent >>= 1; } return result; }

int main()
{
    constexpr long long number = 15;
    const long long power = modularPowerHandBuilt(2, 2, number);
    const long long factorA = gcdHandBuilt(power - 1, number);
    const long long factorB = gcdHandBuilt(power + 1, number);
    QuantumSimulator().run(Algorithms::qft(0, 3));
    std::cout << number << " = " << factorA << " * " << factorB << "\n";
    return 0;
}