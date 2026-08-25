#pragma once

#include "AlgorithmLibrary.hpp"
#include "QuantumSimulator.hpp"

#include <vector>

namespace Algorithms
{
class GroverOracle
{
public:
    GroverOracle(int qubitCount, int markedState);
    void appendTo(QuantumCircuit& circuit) const;
    int qubitCount() const;
    int markedState() const;

private:
    int qubitCount_;
    int markedState_;
};

class GroverOperator
{
public:
    explicit GroverOperator(const GroverOracle& oracle);
    void appendTo(QuantumCircuit& circuit) const;

private:
    GroverOracle oracle_;
};

class GroverSearch
{
public:
    explicit GroverSearch(GroverOracle oracle);
    QuantumCircuit circuit() const;
    SimulationResult run() const;
    int iterations() const;

private:
    GroverOracle oracle_;
};

struct ShorFactors;

class Shor
{
public:
    explicit Shor(long long number);
    ShorFactors factor() const;
    QuantumCircuit periodFindingCircuit(int countingQubits) const;

private:
    long long number_;
};

struct ShorFactors
{
    long long factorA = 0;
    long long factorB = 0;
    bool success = false;
};

long long gcd(long long left, long long right);
long long modularMultiply(long long left, long long right, long long modulus);
long long modularPower(long long base, long long exponent, long long modulus);
long long classicalOrder(long long base, long long modulus);
ShorFactors classicalFactor(long long number);

QuantumCircuit modularExponentiation(long long base, long long modulus, int countingQubits);
QuantumCircuit shorPeriodFindingCircuit(long long base, long long modulus, int countingQubits);
ShorFactors shorFactor(long long number);
}