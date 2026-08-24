#pragma once

#include "QuantumCircuit.hpp"
#include "QuantumSimulator.hpp"

#include <functional>
#include <utility>
#include <vector>

namespace Variational
{
struct PauliTerm
{
    double coefficient;
    std::string paulis;
};

class Hamiltonian
{
public:
    Hamiltonian() = default;
    explicit Hamiltonian(std::vector<PauliTerm> terms);

    double expectation(const std::vector<std::complex<double>>& state) const;
    const std::vector<PauliTerm>& terms() const;

private:
    std::vector<PauliTerm> terms_;
};

using ParameterizedCircuit = std::function<QuantumCircuit(const std::vector<double>&)>;

struct OptimizationResult
{
    std::vector<double> parameters;
    double value = 0.0;
    int iterations = 0;
};

class CoordinateDescent
{
public:
    CoordinateDescent(double initialStep = 0.25, int maxIterations = 100);
    OptimizationResult minimize(
        const std::function<double(const std::vector<double>&)>& cost,
        std::vector<double> initialParameters) const;

private:
    double initialStep_;
    int maxIterations_;
};

class VQE
{
public:
    VQE(ParameterizedCircuit ansatz, Hamiltonian hamiltonian);

    double energy(const std::vector<double>& parameters) const;
    OptimizationResult minimize(
        std::vector<double> initialParameters,
        const CoordinateDescent& optimizer = CoordinateDescent()) const;

private:
    ParameterizedCircuit ansatz_;
    Hamiltonian hamiltonian_;
};

struct Edge
{
    int first;
    int second;
};

class QAOA
{
public:
    QAOA(int qubitCount, std::vector<Edge> edges);

    QuantumCircuit circuit(const std::vector<double>& parameters) const;
    double cost(const std::vector<double>& parameters) const;
    OptimizationResult minimize(
        std::vector<double> initialParameters,
        const CoordinateDescent& optimizer = CoordinateDescent()) const;

private:
    int qubitCount_;
    std::vector<Edge> edges_;
};
}