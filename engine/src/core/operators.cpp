#include "operators.hpp"

#include <stdexcept>
#include <cmath>
#include <limits>

namespace quantum
{

    // =========================================================================
    // Helper Utilities (Internal to Translation Unit)
    // =========================================================================
    namespace
    {
        // Standard Identity Matrix helper for initialization tracking
        Matrix2D createIdentity(std::size_t size)
        {
            Matrix2D mat(size, std::vector<Complex>(size, Complex(0.0, 0.0)));
            for (std::size_t i = 0; i < size; ++i)
            {
                mat[i][i] = Complex(1.0, 0.0);
            }
            return mat;
        }
    }

    // =========================================================================
    // Standard Gates Implementation
    // =========================================================================
    void operators::stampLastOperation(const circuit& source)
    {
        m_gates.back().sourceCircuit = &source;
    }

    // --- X Gate ---
    void operators::x(int qubit)
    {
        gate g;
        g.name = "X";
        g.type = GateType::Standard;
        g.targets = {qubit};
        m_gates.push_back(g);
    }
    void operators::x(const qubit_view& q)
    {
        x(static_cast<int>(q.globalIdx()));
        stampLastOperation(q.context());
    }

    // --- Y Gate ---
    void operators::y(int qubit)
    {
        gate g;
        g.name = "Y";
        g.type = GateType::Standard;
        g.targets = {qubit};
        m_gates.push_back(g);
    }
    void operators::y(const qubit_view& q)
    {
        y(static_cast<int>(q.globalIdx()));
        stampLastOperation(q.context());
    }

    // --- Z Gate ---
    void operators::z(int qubit)
    {
        gate g;
        g.name = "Z";
        g.type = GateType::Standard;
        g.targets = {qubit};
        m_gates.push_back(g);
    }
    void operators::z(const qubit_view& q)
    {
        z(static_cast<int>(q.globalIdx()));
        stampLastOperation(q.context());
    }

    // --- H Gate ---
    void operators::h(int qubit)
    {
        gate g;
        g.name = "H";
        g.type = GateType::Standard;
        g.targets = {qubit};
        m_gates.push_back(g);
    }
    void operators::h(const qubit_view& q)
    {
        h(static_cast<int>(q.globalIdx()));
        stampLastOperation(q.context());
    }

    // --- T Gate ---
    void operators::t(int qubit)
    {
        gate g;
        g.name = "T";
        g.type = GateType::Standard;
        g.targets = {qubit};
        m_gates.push_back(g);
    }
    void operators::t(const qubit_view& q)
    {
        t(static_cast<int>(q.globalIdx()));
        stampLastOperation(q.context());
    }

    // =========================================================================
    // Single Qubit Parameterized Gates
    // =========================================================================

    // --- Rx Gate ---
    void operators::rx(int qubit, double theta)
    {
        gate g;
        g.name = "Rx";
        g.type = GateType::Parameterized;
        g.targets = {qubit};
        g.parameters = {theta};
        m_gates.push_back(g);
    }
    void operators::rx(const qubit_view& q, double theta)
    {
        rx(static_cast<int>(q.globalIdx()), theta);
        stampLastOperation(q.context());
    }

    // --- Ry Gate ---
    void operators::ry(int qubit, double theta)
    {
        gate g;
        g.name = "Ry";
        g.type = GateType::Parameterized;
        g.targets = {qubit};
        g.parameters = {theta};
        m_gates.push_back(g);
    }
    void operators::ry(const qubit_view& q, double theta)
    {
        ry(static_cast<int>(q.globalIdx()), theta);
        stampLastOperation(q.context());
    }

    // --- Rz Gate ---
    void operators::rz(int qubit, double theta)
    {
        gate g;
        g.name = "Rz";
        g.type = GateType::Parameterized;
        g.targets = {qubit};
        g.parameters = {theta};
        m_gates.push_back(g);
    }
    void operators::rz(const qubit_view& q, double theta)
    {
        rz(static_cast<int>(q.globalIdx()), theta);
        stampLastOperation(q.context());
    }

    // =========================================================================
    // Two & Three Qubit Gates
    // =========================================================================

    // --- CX Gate ---
    void operators::cx(int targetQubit, int controlQubit)
    {
        gate g;
        g.name = "CX";
        g.type = GateType::Standard;
        g.targets = {targetQubit};
        g.controls = {controlQubit};
        m_gates.push_back(g);
    }
    void operators::cx(const qubit_view& q_target, const qubit_view& q_control)
    {
        if (&q_target.context() != &q_control.context()) {
            throw std::invalid_argument("Gate target and control must belong to the same circuit");
        }
        cx(static_cast<int>(q_target.globalIdx()), static_cast<int>(q_control.globalIdx()));
        stampLastOperation(q_target.context());
    }

    // --- CY Gate ---
    void operators::cy(int targetQubit, int controlQubit)
    {
        gate g;
        g.name = "CY";
        g.type = GateType::Standard;
        g.targets = {targetQubit};
        g.controls = {controlQubit};
        m_gates.push_back(g);
    }
    void operators::cy(const qubit_view& q_target, const qubit_view& q_control)
    {
        if (&q_target.context() != &q_control.context()) {
            throw std::invalid_argument("Gate target and control must belong to the same circuit");
        }
        cy(static_cast<int>(q_target.globalIdx()), static_cast<int>(q_control.globalIdx()));
        stampLastOperation(q_target.context());
    }

    // --- CZ Gate ---
    void operators::cz(int targetQubit, int controlQubit)
    {
        gate g;
        g.name = "CZ";
        g.type = GateType::Standard;
        g.targets = {targetQubit};
        g.controls = {controlQubit};
        m_gates.push_back(g);
    }
    void operators::cz(const qubit_view& q_target, const qubit_view& q_control)
    {
        if (&q_target.context() != &q_control.context()) {
            throw std::invalid_argument("Gate target and control must belong to the same circuit");
        }
        cz(static_cast<int>(q_target.globalIdx()), static_cast<int>(q_control.globalIdx()));
        stampLastOperation(q_target.context());
    }

    // --- Swap Gate ---
    void operators::swap(int qubit1, int qubit2)
    {
        gate g;
        g.name = "SWAP";
        g.type = GateType::Standard;
        g.targets = {qubit1, qubit2};
        m_gates.push_back(g);
    }
    void operators::swap(const qubit_view& q1, const qubit_view& q2)
    {
        if (&q1.context() != &q2.context()) {
            throw std::invalid_argument("Swap wires must belong to the same circuit");
        }
        swap(static_cast<int>(q1.globalIdx()), static_cast<int>(q2.globalIdx()));
        stampLastOperation(q1.context());
    }

    // --- CCX Gate ---
    void operators::ccx(int targetQubit, int controlQubit1, int controlQubit2)
    {
        gate g;
        g.name = "CCX";
        g.type = GateType::Standard;
        g.targets = {targetQubit};
        g.controls = {controlQubit1, controlQubit2};
        m_gates.push_back(g);
    }
    void operators::ccx(const qubit_view& q_target, const qubit_view& q_control01, const qubit_view& q_control02)
    {
        if (&q_target.context() != &q_control01.context() ||
            &q_target.context() != &q_control02.context()) {
            throw std::invalid_argument("Gate target and controls must belong to the same circuit");
        }
        ccx(static_cast<int>(q_target.globalIdx()),
            static_cast<int>(q_control01.globalIdx()),
            static_cast<int>(q_control02.globalIdx()));
        stampLastOperation(q_target.context());
    }

    // =========================================================================
    // Custom Matrix Operations (Factories)
    // =========================================================================

    void operators::apply(int qubit, const Matrix2D &matrix)
    {
        gate g;
        g.name = "CustomU";
        g.type = GateType::CustomMatrix;
        g.targets = {qubit};
        g.explicitMatrix = matrix;
        m_gates.push_back(g);
    }
    void operators::apply(const qubit_view& q, const Matrix2D &matrix)
    {
        apply(static_cast<int>(q.globalIdx()), matrix);
        stampLastOperation(q.context());
    }

    // =========================================================================
    // Custom Controlled Gate Overloads
    // =========================================================================

    void operators::controlled(int targetQubit, int controlQubit, const Matrix2D &matrix)
    {
        gate g;
        g.name = "CU";
        g.type = GateType::Controlled;
        g.targets = {targetQubit};
        g.controls = {controlQubit};
        g.explicitMatrix = matrix;
        m_gates.push_back(g);
    }
    void operators::controlled(const qubit_view& q_target, const qubit_view& q_control, const Matrix2D &matrix)
    {
        if (&q_target.context() != &q_control.context()) {
            throw std::invalid_argument("Gate target and control must belong to the same circuit");
        }
        controlled(static_cast<int>(q_target.globalIdx()), static_cast<int>(q_control.globalIdx()), matrix);
        stampLastOperation(q_target.context());
    }

    // =========================================================================
    // Custom Multi-Controlled Gate Overloads
    // =========================================================================

    void operators::multi_controlled(int targetQubit, const std::vector<int> &controlQubits, const Matrix2D &matrix)
    {
        gate g;
        g.name = "MCU";
        g.type = GateType::MultiControlled;
        g.targets = {targetQubit};
        g.controls = controlQubits;
        g.explicitMatrix = matrix;
        m_gates.push_back(g);
    }
    void operators::multi_controlled(const qubit_view& q_target, const std::vector<qubit_view> &controlQubits, const Matrix2D &matrix)
    {
        std::vector<int> rawControls;
        for (const auto &cq : controlQubits)
        {
            if (&cq.context() != &q_target.context()) {
                throw std::invalid_argument("Gate target and controls must belong to the same circuit");
            }
            rawControls.push_back(static_cast<int>(cq.globalIdx()));
        }
        multi_controlled(static_cast<int>(q_target.globalIdx()), rawControls, matrix);
        stampLastOperation(q_target.context());
    }
    // =========================================================================
    // Other Workspace Operations (Measurement & Barriers)
    // =========================================================================
    void operators::measure(int qubit, int bit)
    {
        gate g;
        g.name = "MEASURE";
        g.type = GateType::Measurement;
        g.targets = {qubit};
        g.classicalBit = bit;
        m_gates.push_back(g);
    }
    void operators::measure(const qubit_view& q, int bit)
    {
        measure(static_cast<int>(q.globalIdx()), bit);
        stampLastOperation(q.context());
    }
    void operators::measure(const register_view& reg)
    {
        gate g;
        g.name = "MEASURE_REGISTER";
        g.type = GateType::Measurement;
        g.registerId = static_cast<int>(reg.id());
        g.sourceCircuit = &reg.context();
        for (std::size_t local = 0; local < reg.size(); ++local) {
            g.targets.push_back(static_cast<int>(reg.startIndex() + local));
        }
        m_gates.push_back(g);
    }
    void operators::sampler(int qubit, int shots)
    {
        gate g;
        g.name = "SAMPLER";
        g.type = GateType::Sampling;
        g.targets = {qubit};
        g.parameters = {static_cast<double>(shots)};
        m_gates.push_back(g);
    }
    void operators::sampler(const qubit_view& q, int shots)
    {
        sampler(static_cast<int>(q.globalIdx()), shots);
        stampLastOperation(q.context());
    }
    void operators::sampler(const register_view& reg, int shots)
    {
        gate g;
        g.name = "SAMPLER_REGISTER";
        g.type = GateType::Sampling;
        g.parameters = {static_cast<double>(shots)};
        g.registerId = static_cast<int>(reg.id());
        g.sourceCircuit = &reg.context();
        for (std::size_t local = 0; local < reg.size(); ++local) {
            g.targets.push_back(static_cast<int>(reg.startIndex() + local));
        }
        m_gates.push_back(g);
    }
    void operators::barrier(const std::vector<int> &qubits)
    {
        gate g;
        g.name = "BARRIER";
        g.type = GateType::Barrier;
        g.targets = qubits;
        m_gates.push_back(g);
    }
    void operators::barrier(const std::vector<qubit_view> &qubits)
    {
        std::vector<int> rawQubits;
        for (const auto &q : qubits)
        {
            rawQubits.push_back(static_cast<int>(q.globalIdx()));
        }
        barrier(rawQubits);
        if (!qubits.empty()) {
            stampLastOperation(qubits.front().context());
            for (const auto& q : qubits) {
                if (&q.context() != &qubits.front().context()) {
                    m_gates.pop_back();
                    throw std::invalid_argument("Barrier wires must belong to the same circuit");
                }
            }
        }
    }
    // =========================================================================
    // Whole-Register Transformers & Primitive Blocks
    // =========================================================================
    void operators::qft(int registerId)
    {
        gate g;
        g.name = "QFT";
        g.type = GateType::RegisterTransform;
        g.parameters = {static_cast<double>(registerId)};
        g.registerId = registerId;
        m_gates.push_back(g);
    }
    void operators::qft(const register_view& reg)
    {
        // Falls back seamlessly to the structure's dynamic structural ID
        qft(static_cast<int>(reg.id()));
        m_gates.back().sourceCircuit = &reg.context();
        m_gates.back().registerId = static_cast<int>(reg.id());
    }
    void operators::iqft(int registerId)
    {
        gate g;
        g.name = "IQFT";
        g.type = GateType::RegisterTransform;
        g.parameters = {static_cast<double>(registerId)};
        g.registerId = registerId;
        m_gates.push_back(g);
    }
    void operators::iqft(const register_view& reg)
    {
        iqft(static_cast<int>(reg.id()));
        m_gates.back().sourceCircuit = &reg.context();
        m_gates.back().registerId = static_cast<int>(reg.id());
    }
    void operators::walshHadamard(int registerId)
    {
        gate g;
        g.name = "WALSH_HADAMARD";
        g.type = GateType::RegisterTransform;
        g.parameters = {static_cast<double>(registerId)};
        g.registerId = registerId;
        m_gates.push_back(g);
    }
    void operators::walshHadamard(const register_view& reg)
    {
        walshHadamard(static_cast<int>(reg.id()));
        m_gates.back().sourceCircuit = &reg.context();
        m_gates.back().registerId = static_cast<int>(reg.id());
    }
    // =========================================================================
    // Register-Level Diffusion Primitive
    // =========================================================================
    void operators::amplitudeAmplification(int registerId, int iterations)
    {
        gate g;
        g.name = "GROVER_DIFFUSION";
        g.type = GateType::RegisterTransform;
        g.parameters = {static_cast<double>(registerId), static_cast<double>(iterations)};
        g.registerId = registerId;
        m_gates.push_back(g);
    }
    void operators::amplitudeAmplification(const register_view& reg, int iterations)
    {
        amplitudeAmplification(static_cast<int>(reg.id()), iterations);
        m_gates.back().sourceCircuit = &reg.context();
        m_gates.back().registerId = static_cast<int>(reg.id());
    }

    void operators::applyRegister(int registerId, const Matrix2D &matrix)
    {
        gate g;
        g.name = "CustomRegisterU";
        g.type = GateType::CustomRegisterMatrix;
        g.registerId = registerId;
        g.explicitMatrix = matrix;
        m_gates.push_back(g);
    }
    void operators::applyRegister(const register_view& reg, const Matrix2D& matrix)
    {
        applyRegister(static_cast<int>(reg.id()), matrix);
        m_gates.back().sourceCircuit = &reg.context();
        m_gates.back().registerId = static_cast<int>(reg.id());
    }
    void operators::applyControlledRegister(int registerId, int controlQubit, const Matrix2D& matrix)
    {
        gate g;
        g.name = "ControlledCustomRegisterU";
        g.type = GateType::CustomRegisterMatrix;
        g.controls = {controlQubit};
        g.registerId = registerId;
        g.explicitMatrix = matrix;
        m_gates.push_back(g);
    }
    void operators::applyControlledRegister(
        const register_view& reg,
        const qubit_view& controlQubit,
        const Matrix2D& matrix)
    {
        if (&reg.context() != &controlQubit.context()) {
            throw std::invalid_argument("Controlled register and control qubit must belong to the same circuit");
        }
        applyControlledRegister(
            static_cast<int>(reg.id()),
            static_cast<int>(controlQubit.globalIdx()),
            matrix);
        m_gates.back().sourceCircuit = &reg.context();
    }

    // =========================================================================
    // Output Inspection Pipeline Accessors
    // =========================================================================
    const std::vector<gate> &operators::getGates() const
    {
        return m_gates;
    }
    
} // namespace quantum