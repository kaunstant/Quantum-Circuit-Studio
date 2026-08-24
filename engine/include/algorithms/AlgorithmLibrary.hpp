#pragma once

#include "QuantumCircuit.hpp"

#include <functional>
#include <cstddef>

namespace Algorithms
{
using CircuitBuilder = std::function<void(QuantumCircuit&)>;

QuantumCircuit qft(int firstQubit, int qubitCount);
QuantumCircuit inverseQft(int firstQubit, int qubitCount);
QuantumCircuit walshHadamard(int firstQubit, int qubitCount);

QuantumCircuit phaseEstimation(const Matrix& unitary, int countingQubits, int firstTargetQubit);
QuantumCircuit amplitudeAmplification(int qubitCount, const CircuitBuilder& oracle);

QuantumCircuit teleportation();
QuantumCircuit superdenseCoding(int messageBit0, int messageBit1);
QuantumCircuit deutsch(const CircuitBuilder& oracle);
QuantumCircuit deutschJozsa(int inputQubits, const CircuitBuilder& oracle);
QuantumCircuit bernsteinVazirani(const std::vector<int>& secretBits);
QuantumCircuit simon(const std::vector<int>& secretBits);
}