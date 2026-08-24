#pragma once

#include <vector>
#include <complex>
#include <chrono>
#include <thread>

class QuantumCircuit;

class QuantumSimulator
{
public:
    void run(const QuantumCircuit& circuit, std::vector<std::complex<double>>& state, int N);

    std::chrono::duration<double, std::milli> duration{};
};