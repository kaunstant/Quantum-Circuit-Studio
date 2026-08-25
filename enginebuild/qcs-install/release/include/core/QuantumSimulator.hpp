#pragma once

#include <vector>
#include <complex>
#include <chrono>
#include <thread>
#include <random>
#include "QuantumCircuit.hpp"

struct SimulationResult
{
    std::vector<std::complex<double>> state;
    std::vector<int> classicalBits;
};

class QuantumCircuit;

class QuantumSimulator
{
public:
    void run(const QuantumCircuit& circuit, std::vector<std::complex<double>>& state, int N);
    SimulationResult run(const QuantumCircuit& circuit);
    static std::vector<double> probabilities(const std::vector<std::complex<double>>& state);

    std::chrono::duration<double, std::milli> duration{};
};