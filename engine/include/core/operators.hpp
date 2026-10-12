#pragma once

#include "circuit.hpp"

#include <vector>
#include <complex>
#include <cstddef>
#include <string>
#include <variant>

namespace quantum {

// Modern aliasing for high-performance complex scalar fields
using Complex = std::complex<double>;
using Matrix2D = std::vector<std::vector<Complex>>;

enum class GateType {
    Standard,
    Parameterized,
    CustomMatrix,
    Controlled,
    MultiControlled,
    CustomRegisterMatrix,
    Measurement,
    Sampling,
    Barrier,
    RegisterTransform
};

// Unified structural metadata descriptor representing a distinct operation
struct gate {
    std::string name;
    GateType type = GateType::Standard;
    
    // Wire Indices Routing Arrays
    std::vector<int> targets;
    std::vector<int> controls;
    
    // Numerical Properties
    std::vector<double> parameters;
    Matrix2D explicitMatrix; // Left empty for named standard gates, populated for custom matrices
    const circuit* sourceCircuit = nullptr;
    int registerId = -1;
    
    // Classical feedback targeting index for measure routines
    int classicalBit = -1;
};

class operators
{
public:
    // =========================================================================
    // Standard Gates
    // =========================================================================

    // Single Qubit Gates
    void x(int qubit); void x(const qubit_view& q);
    void y(int qubit); void y(const qubit_view& q);
    void z(int qubit); void z(const qubit_view& q);
    void h(int qubit); void h(const qubit_view& q);
    void t(int qubit); void t(const qubit_view& q);

    // Single Qubit Parameterized Gates
    void rx(int qubit, double theta); void rx(const qubit_view& q, double theta);
    void ry(int qubit, double theta); void ry(const qubit_view& q, double theta);
    void rz(int qubit, double theta); void rz(const qubit_view& q, double theta);

    // Two Qubit Gates
    void cx(int targetQubit, int controlQubit); void cx(const qubit_view& q_target, const qubit_view& q_control);
    void cy(int targetQubit, int controlQubit); void cy(const qubit_view& q_target, const qubit_view& q_control);
    void cz(int targetQubit, int controlQubit); void cz(const qubit_view& q_target, const qubit_view& q_control);
    void swap(int qubit1, int qubit2);          void swap(const qubit_view& q1, const qubit_view& q2);
    
    // Three Qubit Gate
    void ccx(int targetQubit, int controlQubit1, int controlQubit2); 
    void ccx(const qubit_view& q_target, const qubit_view& q_control01, const qubit_view& q_control02);

    // =========================================================================
    // Other Gates (Custom Matrices and Factories)
    // =========================================================================

    // Custom Single Qubit Gate Overloads
    void apply(int qubit, const Matrix2D& matrix);
    void apply(const qubit_view& q, const Matrix2D& matrix);

    // Custom Controlled Gates Overloads
    void controlled(int targetQubit, int controlQubit, const Matrix2D& matrix);
    void controlled(const qubit_view& q_target, const qubit_view& q_control, const Matrix2D& matrix);

    // Custom Multi Controlled Gates Overloads
    void multi_controlled(int targetQubit, const std::vector<int>& controlQubits, const Matrix2D& matrix);
    void multi_controlled(const qubit_view& q_target, const std::vector<qubit_view>& controlQubits, const Matrix2D& matrix);

    // =========================================================================
    // Other Operations
    // =========================================================================

    // Measurement Overloads
    void measure(int qubit, int bit);
    void measure(const qubit_view& q, int bit);
    void measure(const register_view& reg);

    // Sample one wire repeatedly without collapsing the simulated state.
    void sampler(int qubit, int shots);
    void sampler(const qubit_view& q, int shots);
    void sampler(const register_view& reg, int shots);
    
    // Barrier Overloads
    void barrier(const std::vector<int>& qubits = {});
    void barrier(const std::vector<qubit_view>& qubits);

    // =========================================================================
    // Register Operators & Algorithms
    // =========================================================================

    // Transformations (Accepts target sequence ID layout boundaries OR active registers)
    void qft(int registerId);                  void qft(const register_view& reg);
    void iqft(int registerId);                 void iqft(const register_view& reg);
    void walshHadamard(int registerId);        void walshHadamard(const register_view& reg);

    // Register-level transform and diffusion primitives.
    void amplitudeAmplification(int registerId, int iterations);
    void amplitudeAmplification(const register_view& reg, int iterations);

    // Apply a unitary matrix over all basis states of a selected register.
    void applyRegister(int registerId, const Matrix2D& matrix);
    void applyRegister(const register_view& reg, const Matrix2D& matrix);
    
    void applyControlledRegister(int registerId, int controlQubit, const Matrix2D& matrix);
    void applyControlledRegister(
        const register_view& reg,
        const qubit_view& controlQubit,
        const Matrix2D& matrix);

    // Timeline Pipeline Export Accessor
    const std::vector<gate>& getGates() const;

private:
    void stampLastOperation(const circuit& source);
    std::vector<gate> m_gates;
};

} // namespace quantum
