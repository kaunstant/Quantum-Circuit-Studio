#pragma once

#include "QuantumRegister.hpp"
#include "config.hpp"

#include <vector>
#include <complex>
#include <string>

using Matrix = std::vector<std::vector<std::complex<double>>>;

// =====================================================
// Operation model
// =====================================================

enum class GateType
{
    X, Y, Z,
    H, T,
    RX, RY, RZ,
    CX, CY, CZ,
    SWAP,
    CCX
};

enum class OperationType
{
    Gate,
    Matrix,
    ControlledMatrix,
    MultiControlledX,
    Measurement,
    Barrier
};

class Parameter
{
public:
    explicit Parameter(std::string name);
    const std::string& name() const;

private:
    std::string name_;
};

// =====================================================
// Gate Definition
// =====================================================

struct Gate
{
    GateType type = GateType::X;
    OperationType operation = OperationType::Gate;

    int qubit1 = -1; // target
    int qubit2 = -1; // control
    int qubit3 = -1; // control

    double theta = 0.0;
    std::string parameter;
    std::vector<int> qubits;
    Matrix matrix;
    std::vector<int> controls;
    int classicalBit = -1;
};

class QuantumCircuit
{
public:
    QuantumCircuit(int numQubits);

    int numQubits() const;
    void append(const QuantumCircuit& circuit);
    QuantumCircuit operator+(const QuantumCircuit& circuit) const;
    void append(Register::QuantumRegister registerIdx);
    void append(Qubit qubitIdx);

    void X(int qubit);
    void Y(int qubit);
    void Z(int qubit);

    void H(int qubit);
    void T(int qubit);

    void RX(int qubit, double theta);
    void RY(int qubit, double theta);
    void RZ(int qubit, double theta);

    void CX(int targetQubit, int controlQubit);
    void CY(int targetQubit, int controlQubit);
    void CZ(int targetQubit, int controlQubit);

    void SWAP(int qubit1, int qubit2);

    void CCX(int targetQubit, int controlQubit1, int controlQubit2);

    void X(Qubit qubit); void Y(Qubit qubit); void Z(Qubit qubit);
    void H(Qubit qubit); void T(Qubit qubit);
    void RX(Qubit qubit, double theta); void RY(Qubit qubit, double theta); void RZ(Qubit qubit, double theta);
    void X(const Register::QuantumRegister& reg); void Y(const Register::QuantumRegister& reg);
    void Z(const Register::QuantumRegister& reg); void H(const Register::QuantumRegister& reg);
    void T(const Register::QuantumRegister& reg);
    void RX(const Register::QuantumRegister& reg, double theta);
    void RY(const Register::QuantumRegister& reg, double theta);
    void RZ(const Register::QuantumRegister& reg, double theta);

    void RX(int qubit, const Parameter& parameter);
    void RY(int qubit, const Parameter& parameter);
    void RZ(int qubit, const Parameter& parameter);
    void bind(const Parameter& parameter, double value);
    void bind(const std::string& parameter, double value);

    void apply(const Matrix& matrix, const std::vector<int>& qubits);
    void controlled(const Matrix& matrix, int control, const std::vector<int>& targetQubits);
    void multiControlledX(const std::vector<int>& controls, int targetQubit);
    void controlledPhase(int control, int targetQubit, double angle);
    void measure(int qubit, int classicalBit);
    void measure(const Register::QuantumRegister& quantum, const Register::ClassicalRegister& classical);
    void barrier(const std::vector<int>& qubits = {});

    // Lowercase spellings match the examples in the v0.2 manual.
    void x(int qubit); void y(int qubit); void z(int qubit); void h(int qubit); void t(int qubit);
    void rx(int qubit, double theta); void ry(int qubit, double theta); void rz(int qubit, double theta);
    void cx(int targetQubit, int controlQubit); void cy(int targetQubit, int controlQubit); void cz(int targetQubit, int controlQubit);
    void swap(int qubit1, int qubit2); void ccx(int targetQubit, int controlQubit1, int controlQubit2);
    void x(Qubit q); void y(Qubit q); void z(Qubit q); void h(Qubit q); void t(Qubit q);
    void x(const Register::QuantumRegister& r); void y(const Register::QuantumRegister& r);
    void z(const Register::QuantumRegister& r); void h(const Register::QuantumRegister& r); void t(const Register::QuantumRegister& r);

    const std::vector<Gate>& getGates() const;

private:
    int m_numQubits;

    std::vector<Gate> m_gates;

    static Config::RegisterID global_qubit_index;

    void addSingleQubitGate(GateType type, int qubit, double theta = 0.0, const std::string& parameter = {});
    void validateQubit(int qubit) const;
    void validateDistinct(const std::vector<int>& qubits) const;
};