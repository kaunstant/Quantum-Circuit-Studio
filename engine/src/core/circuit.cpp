#include "circuit.hpp"

#include <iostream>
#include <utility>
#include <stdexcept>

namespace quantum
{

    circuit::circuit(std::size_t initialQubitCount)
    {
        // Add initial wires as standalone components at the beginning of the layout.
        m_components.reserve(initialQubitCount);
        for (std::size_t i = 0; i < initialQubitCount; ++i)
        {
            m_components.push_back(qubit());
            m_globalQubitCounter += 1;
        }
    }

    void circuit::append(::qubit q)
    {
        // A standalone qubit occupies one new global wire position.
        m_components.push_back(std::move(q));
        m_globalQubitCounter += 1;
    }

    void circuit::append(::registers::q_register reg)
    {
        // A register occupies a consecutive block of global wire positions.
        const std::size_t registerSize = reg.size();
        m_components.push_back(std::move(reg));
        m_registerCounter += 1;
        m_globalQubitCounter += registerSize;
    }

    circuit &circuit::operator+=(const circuit &other)
    {
        // Preserve the source component order while adding its elements to this circuit.
        const circuit source(other);
        m_components.reserve(m_components.size() + source.m_components.size());
        for (const auto &comp : source.m_components)
        {
            std::visit([this](const auto &item)
                       {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, qubit>) {
                m_components.push_back(item);
                m_globalQubitCounter += 1;
            } else if constexpr (std::is_same_v<T, registers::q_register>) {
                const std::size_t registerSize = item.size();
                m_components.push_back(item);
                m_registerCounter += 1;
                m_globalQubitCounter += registerSize;
            } }, comp);
        }
        return *this;
    }

    circuit operator+(circuit lhs, const circuit &rhs)
    {
        // Reuse append-assignment on the left-hand copy.
        lhs += rhs;
        return lhs;
    }

    // Return the running number of global wire positions.
    std::size_t circuit::numQubits() const { return m_globalQubitCounter; }

    // Return the number of register blocks added to the circuit.
    std::size_t circuit::numRegisters() const { return m_registerCounter; }

    // Print standalone wires individually and registers as grouped wire blocks.
    // Example: [q0, q1, [q2, q3, q4], q5, [q6, q7]]
    void circuit::qubits() const
    {
        std::cout << "[";
        std::size_t runningQubits = 0;
        for (std::size_t i = 0; i < m_components.size(); ++i)
        {
            std::visit([&runningQubits](const auto &item)
                       {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, qubit>) {
                std::cout << "q" << runningQubits;
                runningQubits += 1;
            } else if constexpr (std::is_same_v<T, registers::q_register>) {
                std::cout << "[";
                for (std::size_t j = 0; j < item.size(); ++j) {
                    std::cout << "q" << (runningQubits + j);
                    if (j < item.size() - 1) std::cout << ", ";
                }
                std::cout << "]";
                runningQubits += item.size();
            } }, m_components[i]);
            if (i < m_components.size() - 1)
                std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }

    // Print each register and the consecutive global wires that it contains.
    // Example: [reg0 {q2, q3, q4}, reg1 {q6, q7}]
    void circuit::registers() const
    {
        std::cout << "[";
        std::size_t runningQubits = 0;
        std::size_t regIdCounter = 0;
        bool firstReg = true;

        for (std::size_t i = 0; i < m_components.size(); ++i)
        {
            std::visit([&runningQubits, &regIdCounter, &firstReg](const auto &item)
                       {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, qubit>) {
                runningQubits += 1;
            } else if constexpr (std::is_same_v<T, registers::q_register>) {
                if (!firstReg) std::cout << ", ";
                firstReg = false;
                std::cout << "reg" << regIdCounter << " {";
                for (std::size_t j = 0; j < item.size(); ++j) {
                    std::cout << "q" << (runningQubits + j);
                    if (j < item.size() - 1) std::cout << ", ";
                }
                std::cout << "}";
                runningQubits += item.size();
                regIdCounter += 1;
            } }, m_components[i]);
        }
        std::cout << "]" << std::endl;
    }

    qubit_view circuit::operator[](std::size_t globalIdx) const
    {
        // Check against the current circuit size before creating a view.
        if (globalIdx >= m_globalQubitCounter)
            throw std::out_of_range("Global Index out of bounds!");
        return qubit_view(globalIdx, *this);
    }

    register_view circuit::operator()(std::size_t registerId) const
    {
        // Check against the current register count before creating a view.
        if (registerId >= m_registerCounter)
            throw std::out_of_range("Register ID out of bounds!");
        return register_view(registerId, *this);
    }

    circuit::IndexResolution circuit::resolveGlobalIdx(std::size_t globalIdx) const
    {
        IndexResolution res;
        std::size_t runningQubits = 0;
        std::size_t regCounter = 0;

        // Walk the mixed component sequence while tracking its flattened wire offsets.
        for (const auto &comp : m_components)
        {
            if (std::holds_alternative<qubit>(comp))
            {
                if (runningQubits == globalIdx)
                    return res;
                runningQubits += 1;
            }
            else if (std::holds_alternative<registers::q_register>(comp))
            {
                const auto &reg = std::get<registers::q_register>(comp);
                if (globalIdx >= runningQubits && globalIdx < (runningQubits + reg.size()))
                {
                    res.inRegister = true;
                    res.registerId = regCounter;
                    res.localIdx = globalIdx - runningQubits;
                    return res;
                }
                runningQubits += reg.size();
                regCounter += 1;
            }
        }
        return res;
    }

    // Cache the global position and its register metadata when constructing the view.
    qubit_view::qubit_view(std::size_t globalIdx, const circuit &ctx)
        : m_globalIdx(globalIdx), m_ctx(ctx), m_meta(ctx.resolveGlobalIdx(globalIdx)) {}

    // Generate the display label from the global circuit position.
    std::string qubit_view::name() const
    {
        return "q" + std::to_string(m_globalIdx);
    }
    std::size_t qubit_view::globalIdx() const { return m_globalIdx; }
    bool qubit_view::inRegister() const { return m_meta.inRegister; }
    std::size_t qubit_view::registerId() const { return m_meta.registerId; }
    std::size_t qubit_view::localIdx() const { return m_meta.localIdx; }
    const circuit &qubit_view::context() const noexcept { return m_ctx; }

    // Resolve and retain the selected register's size and global starting position.
    register_view::register_view(std::size_t registerId, const circuit &ctx)
        : m_registerId(registerId), m_ctx(ctx)
    {
        std::size_t runningQubits = 0;
        std::size_t regCounter = 0;
        for (const auto &comp : ctx.m_components)
        {
            if (std::holds_alternative<qubit>(comp))
            {
                runningQubits += 1;
            }
            else if (std::holds_alternative<registers::q_register>(comp))
            {
                const auto &reg = std::get<registers::q_register>(comp);
                if (regCounter == m_registerId)
                {
                    m_size = reg.size();
                    m_startIndex = runningQubits;
                    break;
                }
                runningQubits += reg.size();
                regCounter += 1;
            }
        }
    }

    std::string register_view::name() const { return "reg" + std::to_string(m_registerId); }
    std::size_t register_view::id() const { return m_registerId; }
    std::size_t register_view::size() const { return m_size; }
    std::size_t register_view::startIndex() const { return m_startIndex; }
    const circuit &register_view::context() const noexcept { return m_ctx; }

    qubit_view register_view::operator[](std::size_t localIdx) const
    {
        // Translate the register-local offset into a global wire position.
        if (localIdx >= m_size)
            throw std::out_of_range("Local Index out of bounds!");
        return qubit_view(m_startIndex + localIdx, m_ctx);
    }

} // namespace quantum
