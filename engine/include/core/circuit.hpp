#pragma once

#include "qubit.hpp"
#include "q_register.hpp"

#include <vector>
#include <string>
#include <cstddef>
#include <variant>

namespace quantum {

class qubit_view;
class register_view;

// Owns an ordered sequence of standalone qubits and registers.
class circuit 
{
public:
    // Each entry represents one standalone qubit or one complete register.
    using Component = std::variant<::qubit, ::registers::q_register>;

    // Create a circuit with the requested number of leading standalone qubits.
    explicit circuit(std::size_t initialQubitCount = 0);

    // Add a standalone qubit or register to the end of the circuit.
    void append(::qubit q);
    void append(::registers::q_register reg);

    // Append every component from another circuit in chronological order.
    circuit& operator+=(const circuit& other);
    friend circuit operator+(circuit lhs, const circuit& rhs);

    // Report the number of wires and register blocks currently in the circuit.
    std::size_t numQubits() const;
    std::size_t numRegisters() const;

    // Print the circuit's wire layout or its register blocks to standard output.
    void qubits() const;
    void registers() const;

    // Create read-only views for a global wire index or register ID.
    qubit_view operator[](std::size_t globalIdx) const;
    register_view operator()(std::size_t registerId) const;

private:
    // Views use circuit internals to resolve their requested positions.
    friend class qubit_view;
    friend class register_view;

    // Running totals used to validate lookups and assign positions as components are added.
    std::size_t m_globalQubitCounter = 0;
    std::size_t m_registerCounter = 0;

    // Components remain in the order in which they were added.
    std::vector<Component> m_components;

    // Metadata resolved from a global wire position.
    struct IndexResolution {
        bool inRegister = false;
        std::size_t registerId = static_cast<std::size_t>(-1);
        std::size_t localIdx = static_cast<std::size_t>(-1);
    };

    // Locate the register and local offset containing a global wire, if any.
    IndexResolution resolveGlobalIdx(std::size_t globalIdx) const;
};

// Read-only metadata view for a qubit position in a circuit.
class qubit_view {
public:
    qubit_view(std::size_t globalIdx, const circuit& ctx);
    std::string name() const; // Return the global wire label in qN form.
    std::size_t globalIdx() const;
    bool inRegister() const;
    std::size_t registerId() const;
    std::size_t localIdx() const;
    const circuit& context() const noexcept;
private:
    std::size_t m_globalIdx;
    const circuit& m_ctx;
    circuit::IndexResolution m_meta;
};

// Read-only metadata view for a register; its circuit must outlive the view.
class register_view {
public:
    register_view(std::size_t registerId, const circuit& ctx);
    std::string name() const;
    std::size_t id() const;
    std::size_t size() const;
    std::size_t startIndex() const;
    const circuit& context() const noexcept;
    qubit_view operator[](std::size_t localIdx) const;
private:
    std::size_t m_registerId;
    const circuit& m_ctx;
    std::size_t m_size = 0;
    std::size_t m_startIndex = static_cast<std::size_t>(-1);
};

} // namespace quantum
