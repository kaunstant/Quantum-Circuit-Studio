#pragma once

#include <vector>

enum class GateType
{
    X, Y, Z,
    H, T,
    RX, RY, RZ,
    CX, CY, CZ,
    SWAP,
    CCX
};

struct Gate
{
    GateType type;

    int qubit1 = -1; // target
    int qubit2 = -1; // control
    int qubit3 = -1; // control

    double theta = 0.0;
};



class QuantumCircuit
{
public:
    QuantumCircuit(int numQubits);

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

    const std::vector<Gate>& getGates() const;

private:
    int m_numQubits;

    std::vector<Gate> m_gates;
};