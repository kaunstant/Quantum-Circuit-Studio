#include "QuantumCircuit.h"

#include <iostream>
#include <vector>

QuantumCircuit::QuantumCircuit(int numQubits)
{
    m_numQubits = numQubits;
}

// Single Qubit : Pauli-X (NOT) Gate
void QuantumCircuit::X(int qubit)
{
    m_gates.push_back({GateType::X, qubit});
    std::cout << "Applied X gate on q"
              << qubit
              << '\n';
}

// Single Qubit : Pauli-Y Gate
void QuantumCircuit::Y(int qubit)
{
    m_gates.push_back({GateType::Y, qubit});
    std::cout << "Applied Y gate on q"
              << qubit
              << '\n';
}

// Single Qubit : Pauli-Z Gate
void QuantumCircuit::Z(int qubit)
{
    m_gates.push_back({GateType::Z, qubit});
    std::cout << "Applied Z gate on q"
              << qubit
              << '\n';
}

// Single Qubit (Clifford) : Hadamard Gate
void QuantumCircuit::H(int qubit)
{
    m_gates.push_back({GateType::H, qubit});
    std::cout << "Applied H gate on q"
              << qubit
              << '\n';
}

// Single Qubit (Universal) : T Gate
void QuantumCircuit::T(int qubit)
{
    m_gates.push_back({GateType::T, qubit});
    std::cout << "Applied T gate on q"
              << qubit
              << '\n';
}

// Continous Rotation Gate :  RX
void QuantumCircuit::RX(int qubit, double theta)
{
    m_gates.push_back({GateType::RX, qubit, -1, -1, theta});
    std::cout << "Applied RX(" << theta
              << ") gate on q"
              << qubit
              << '\n';
}

// Continous Rotation Gate :  RY
void QuantumCircuit::RY(int qubit, double theta)
{
    m_gates.push_back({GateType::RY, qubit, -1, -1, theta});
    std::cout << "Applied RY(" << theta
              << ") gate on q"
              << qubit
              << '\n';
}

// Continous Rotation Gate :  RZ
void QuantumCircuit::RZ(int qubit, double theta)
{
    m_gates.push_back({GateType::RZ, qubit, -1, -1, theta});
    std::cout << "Applied RZ(" << theta
              << ") gate on q"
              << qubit
              << '\n';
}

// Two Qubit : CNOT Gate (CX)
void QuantumCircuit::CX(int targetQubit, int controlQubit)
{
    m_gates.push_back({GateType::CX, targetQubit, controlQubit, -1, 0.0});
    std::cout << "Applied CX gate with control q"
              << controlQubit
              << " and target q"
              << targetQubit
              << '\n';
}

// Two Qubit : CY Gate
void QuantumCircuit::CY(int targetQubit, int controlQubit)
{
    m_gates.push_back({GateType::CY, targetQubit, controlQubit, -1, 0.0});
    std::cout << "Applied CY gate with control q"
              << controlQubit
              << " and target q"
              << targetQubit
              << '\n';
}

// Two Qubit : CZ Gate
void QuantumCircuit::CZ(int targetQubit, int controlQubit)
{
    m_gates.push_back({GateType::CZ, targetQubit, controlQubit, -1, 0.0});
    std::cout << "Applied CZ gate with control q"
              << controlQubit
              << " and target q"
              << targetQubit
              << '\n';
}

// SWAP Gate
void QuantumCircuit::SWAP(int qubit1, int qubit2)
{
    m_gates.push_back({GateType::SWAP, qubit1, qubit2, -1, 0.0});
    std::cout << "Applied SWAP gate between q"
              << qubit1
              << " and q"
              << qubit2
              << '\n';
}

// Three-Qubit : Toffoli Gate (CCX)
void QuantumCircuit::CCX(int targetQubit, int controlQubit1, int controlQubit2)
{
    m_gates.push_back({GateType::CCX, targetQubit, controlQubit1, controlQubit2, 0.0});
    std::cout << "Applied CCX gate with control q"
              << controlQubit1
              << ", control q"
              << controlQubit2
              << " and target q"
              << targetQubit
              << '\n';
}

const std::vector<Gate>&
QuantumCircuit::getGates() const
{
    return m_gates;
}