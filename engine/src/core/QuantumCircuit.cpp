#include "QuantumCircuit.hpp"

#include <stdexcept>
#include <utility>

// =============================
// Parameter
// =============================

Parameter::Parameter(std::string name)
    : name_(std::move(name))
{
    if (name_.empty())
    {
        throw std::invalid_argument("Parameter name cannot be empty.");
    }
}

const std::string& Parameter::name() const
{
    return name_;
}

// =============================
// Circuit construction
// =============================

QuantumCircuit::QuantumCircuit(int numQubits)
    : m_numQubits(numQubits)
{
    if (numQubits <= 0 ||
        static_cast<std::size_t>(numQubits) > Config::DEFAULT_MAX_QUBITS)
    {
        throw std::invalid_argument(
            "QuantumCircuit qubit count is outside the supported range.");
    }
}

int QuantumCircuit::numQubits() const
{
    return m_numQubits;
}

void QuantumCircuit::validateQubit(int qubit) const
{
    if (qubit < 0 || qubit >= m_numQubits)
    {
        throw std::out_of_range("Qubit index is outside the circuit.");
    }
}

void QuantumCircuit::validateDistinct(const std::vector<int>& qubits) const
{
    for (std::size_t index = 0; index < qubits.size(); ++index)
    {
        validateQubit(qubits[index]);

        for (std::size_t previous = 0; previous < index; ++previous)
        {
            if (qubits[index] == qubits[previous])
            {
                throw std::invalid_argument(
                    "An operation cannot address a qubit twice.");
            }
        }
    }
}

void QuantumCircuit::addSingleQubitGate(
    GateType type,
    int qubit,
    double theta,
    const std::string& parameter)
{
    validateQubit(qubit);

    Gate gate;
    gate.type = type;
    gate.qubit1 = qubit;
    gate.theta = theta;
    gate.parameter = parameter;
    gate.qubits = {qubit};

    m_gates.push_back(std::move(gate));
}

// =============================
// Single-qubit gates
// =============================

void QuantumCircuit::X(int qubit)
{
    addSingleQubitGate(GateType::X, qubit);
}

void QuantumCircuit::Y(int qubit)
{
    addSingleQubitGate(GateType::Y, qubit);
}

void QuantumCircuit::Z(int qubit)
{
    addSingleQubitGate(GateType::Z, qubit);
}

void QuantumCircuit::H(int qubit)
{
    addSingleQubitGate(GateType::H, qubit);
}

void QuantumCircuit::T(int qubit)
{
    addSingleQubitGate(GateType::T, qubit);
}

void QuantumCircuit::RX(int qubit, double theta)
{
    addSingleQubitGate(GateType::RX, qubit, theta);
}

void QuantumCircuit::RY(int qubit, double theta)
{
    addSingleQubitGate(GateType::RY, qubit, theta);
}

void QuantumCircuit::RZ(int qubit, double theta)
{
    addSingleQubitGate(GateType::RZ, qubit, theta);
}

// =============================
// Controlled gates
// =============================

void QuantumCircuit::CX(int target, int control)
{
    validateDistinct({target, control});

    Gate gate;
    gate.type = GateType::CX;
    gate.qubit1 = target;
    gate.qubit2 = control;
    gate.qubits = {target, control};

    m_gates.push_back(std::move(gate));
}

void QuantumCircuit::CY(int target, int control)
{
    validateDistinct({target, control});

    Gate gate;
    gate.type = GateType::CY;
    gate.qubit1 = target;
    gate.qubit2 = control;
    gate.qubits = {target, control};

    m_gates.push_back(std::move(gate));
}

void QuantumCircuit::CZ(int target, int control)
{
    validateDistinct({target, control});

    Gate gate;
    gate.type = GateType::CZ;
    gate.qubit1 = target;
    gate.qubit2 = control;
    gate.qubits = {target, control};

    m_gates.push_back(std::move(gate));
}

void QuantumCircuit::SWAP(int first, int second)
{
    validateDistinct({first, second});

    Gate gate;
    gate.type = GateType::SWAP;
    gate.qubit1 = first;
    gate.qubit2 = second;
    gate.qubits = {first, second};

    m_gates.push_back(std::move(gate));
}

void QuantumCircuit::CCX(int target, int control1, int control2)
{
    validateDistinct({target, control1, control2});

    Gate gate;
    gate.type = GateType::CCX;
    gate.qubit1 = target;
    gate.qubit2 = control1;
    gate.qubit3 = control2;
    gate.qubits = {target, control1, control2};

    m_gates.push_back(std::move(gate));
}

void QuantumCircuit::multiControlledX(
    const std::vector<int>& controls,
    int target)
{
    std::vector<int> allQubits = controls;
    allQubits.push_back(target);
    validateDistinct(allQubits);

    if (controls.empty())
    {
        X(target);
        return;
    }

    Gate gate;
    gate.operation = OperationType::MultiControlledX;
    gate.qubit1 = target;
    gate.controls = controls;
    gate.qubits = allQubits;

    m_gates.push_back(std::move(gate));
}

void QuantumCircuit::controlledPhase(int control, int target, double angle)
{
    validateDistinct({control, target});

    Gate gate;
    gate.type = GateType::CZ;
    gate.qubit1 = target;
    gate.qubit2 = control;
    gate.theta = angle;
    gate.qubits = {target, control};

    m_gates.push_back(std::move(gate));
}

// =============================
// Qubit and register overloads
// =============================

void QuantumCircuit::X(Qubit qubit) { X(static_cast<int>(qubit.index())); }
void QuantumCircuit::Y(Qubit qubit) { Y(static_cast<int>(qubit.index())); }
void QuantumCircuit::Z(Qubit qubit) { Z(static_cast<int>(qubit.index())); }
void QuantumCircuit::H(Qubit qubit) { H(static_cast<int>(qubit.index())); }
void QuantumCircuit::T(Qubit qubit) { T(static_cast<int>(qubit.index())); }

void QuantumCircuit::RX(Qubit qubit, double theta)
{
    RX(static_cast<int>(qubit.index()), theta);
}

void QuantumCircuit::RY(Qubit qubit, double theta)
{
    RY(static_cast<int>(qubit.index()), theta);
}

void QuantumCircuit::RZ(Qubit qubit, double theta)
{
    RZ(static_cast<int>(qubit.index()), theta);
}

void QuantumCircuit::X(const Register::QuantumRegister& reg)
{
    for (std::size_t index = 0; index < reg.size(); ++index) X(reg[index]);
}

void QuantumCircuit::Y(const Register::QuantumRegister& reg)
{
    for (std::size_t index = 0; index < reg.size(); ++index) Y(reg[index]);
}

void QuantumCircuit::Z(const Register::QuantumRegister& reg)
{
    for (std::size_t index = 0; index < reg.size(); ++index) Z(reg[index]);
}

void QuantumCircuit::H(const Register::QuantumRegister& reg)
{
    for (std::size_t index = 0; index < reg.size(); ++index) H(reg[index]);
}

void QuantumCircuit::T(const Register::QuantumRegister& reg)
{
    for (std::size_t index = 0; index < reg.size(); ++index) T(reg[index]);
}

void QuantumCircuit::RX(const Register::QuantumRegister& reg, double theta)
{
    for (std::size_t index = 0; index < reg.size(); ++index) RX(reg[index], theta);
}

void QuantumCircuit::RY(const Register::QuantumRegister& reg, double theta)
{
    for (std::size_t index = 0; index < reg.size(); ++index) RY(reg[index], theta);
}

void QuantumCircuit::RZ(const Register::QuantumRegister& reg, double theta)
{
    for (std::size_t index = 0; index < reg.size(); ++index) RZ(reg[index], theta);
}

// =============================
// Parameter binding
// =============================

void QuantumCircuit::RX(int qubit, const Parameter& parameter)
{
    addSingleQubitGate(GateType::RX, qubit, 0.0, parameter.name());
}

void QuantumCircuit::RY(int qubit, const Parameter& parameter)
{
    addSingleQubitGate(GateType::RY, qubit, 0.0, parameter.name());
}

void QuantumCircuit::RZ(int qubit, const Parameter& parameter)
{
    addSingleQubitGate(GateType::RZ, qubit, 0.0, parameter.name());
}

void QuantumCircuit::bind(const Parameter& parameter, double value)
{
    bind(parameter.name(), value);
}

void QuantumCircuit::bind(const std::string& name, double value)
{
    bool found = false;

    for (auto& gate : m_gates)
    {
        if (gate.parameter == name)
        {
            gate.theta = value;
            gate.parameter.clear();
            found = true;
        }
    }

    if (!found)
    {
        throw std::invalid_argument(
            "Parameter is not present in this circuit: " + name);
    }
}

// =============================
// Matrix, measurement, and barriers
// =============================

void QuantumCircuit::apply(
    const Matrix& matrix,
    const std::vector<int>& qubits)
{
    validateDistinct(qubits);

    const std::size_t dimension = std::size_t{1} << qubits.size();
    if (matrix.size() != dimension)
    {
        throw std::invalid_argument(
            "Operator dimension does not match its qubit count.");
    }

    for (const auto& row : matrix)
    {
        if (row.size() != dimension)
        {
            throw std::invalid_argument("Operator must be square.");
        }
    }

    Gate gate;
    gate.operation = OperationType::Matrix;
    gate.qubits = qubits;
    gate.matrix = matrix;
    m_gates.push_back(std::move(gate));
}

void QuantumCircuit::controlled(
    const Matrix& matrix,
    int control,
    const std::vector<int>& targets)
{
    std::vector<int> allQubits = {control};
    allQubits.insert(allQubits.end(), targets.begin(), targets.end());
    validateDistinct(allQubits);

    const std::size_t dimension = std::size_t{1} << targets.size();
    if (matrix.size() != dimension)
    {
        throw std::invalid_argument(
            "Controlled operator dimension does not match its targets.");
    }

    for (const auto& row : matrix)
    {
        if (row.size() != dimension)
        {
            throw std::invalid_argument("Operator must be square.");
        }
    }

    Gate gate;
    gate.operation = OperationType::ControlledMatrix;
    gate.qubit1 = control;
    gate.qubits = targets;
    gate.matrix = matrix;
    m_gates.push_back(std::move(gate));
}

void QuantumCircuit::measure(int qubit, int classicalBit)
{
    validateQubit(qubit);

    if (classicalBit < 0)
    {
        throw std::out_of_range("Classical bit index cannot be negative.");
    }

    Gate gate;
    gate.operation = OperationType::Measurement;
    gate.qubit1 = qubit;
    gate.classicalBit = classicalBit;
    gate.qubits = {qubit};
    m_gates.push_back(std::move(gate));
}

void QuantumCircuit::measure(
    const Register::QuantumRegister& quantum,
    const Register::ClassicalRegister& classical)
{
    if (quantum.size() != classical.size())
    {
        throw std::invalid_argument(
            "Quantum and classical registers must have equal size.");
    }

    for (std::size_t index = 0; index < quantum.size(); ++index)
    {
        measure(
            static_cast<int>(quantum[index].index()),
            static_cast<int>(classical.startIndex() + index));
    }
}

void QuantumCircuit::barrier(const std::vector<int>& qubits)
{
    if (!qubits.empty())
    {
        validateDistinct(qubits);
    }

    Gate gate;
    gate.operation = OperationType::Barrier;
    gate.qubits = qubits;
    m_gates.push_back(std::move(gate));
}

// =============================
// Lowercase compatibility API
// =============================

void QuantumCircuit::x(int qubit) { X(qubit); }
void QuantumCircuit::y(int qubit) { Y(qubit); }
void QuantumCircuit::z(int qubit) { Z(qubit); }
void QuantumCircuit::h(int qubit) { H(qubit); }
void QuantumCircuit::t(int qubit) { T(qubit); }

void QuantumCircuit::rx(int qubit, double theta) { RX(qubit, theta); }
void QuantumCircuit::ry(int qubit, double theta) { RY(qubit, theta); }
void QuantumCircuit::rz(int qubit, double theta) { RZ(qubit, theta); }

void QuantumCircuit::cx(int target, int control) { CX(target, control); }
void QuantumCircuit::cy(int target, int control) { CY(target, control); }
void QuantumCircuit::cz(int target, int control) { CZ(target, control); }
void QuantumCircuit::swap(int first, int second) { SWAP(first, second); }
void QuantumCircuit::ccx(int target, int control1, int control2) { CCX(target, control1, control2); }

void QuantumCircuit::x(Qubit qubit) { X(qubit); }
void QuantumCircuit::y(Qubit qubit) { Y(qubit); }
void QuantumCircuit::z(Qubit qubit) { Z(qubit); }
void QuantumCircuit::h(Qubit qubit) { H(qubit); }
void QuantumCircuit::t(Qubit qubit) { T(qubit); }

void QuantumCircuit::x(const Register::QuantumRegister& reg) { X(reg); }
void QuantumCircuit::y(const Register::QuantumRegister& reg) { Y(reg); }
void QuantumCircuit::z(const Register::QuantumRegister& reg) { Z(reg); }
void QuantumCircuit::h(const Register::QuantumRegister& reg) { H(reg); }
void QuantumCircuit::t(const Register::QuantumRegister& reg) { T(reg); }

// =============================
// Circuit composition
// =============================

void QuantumCircuit::append(Register::QuantumRegister reg)
{
    if (reg.startIndex() + reg.size() > static_cast<std::size_t>(m_numQubits))
    {
        throw std::out_of_range("Quantum register does not fit in the circuit.");
    }
}

void QuantumCircuit::append(Qubit qubit)
{
    validateQubit(static_cast<int>(qubit.index()));
}

void QuantumCircuit::append(const QuantumCircuit& circuit)
{
    if (circuit.m_numQubits != m_numQubits)
    {
        throw std::invalid_argument(
            "Circuits must have the same number of qubits.");
    }

    m_gates.insert(
        m_gates.end(),
        circuit.m_gates.begin(),
        circuit.m_gates.end());
}

QuantumCircuit QuantumCircuit::operator+(const QuantumCircuit& circuit) const
{
    QuantumCircuit result(*this);
    result.append(circuit);
    return result;
}

const std::vector<Gate>& QuantumCircuit::getGates() const
{
    return m_gates;
}