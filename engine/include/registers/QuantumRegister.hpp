#pragma once

#include "config.hpp"
#include "Qubit.hpp"

#include <string>
#include <cstddef>

namespace Register
{
// Represents a collection of quantum bits.
class QuantumRegister
{
private:
    std::string name_;
    std::size_t size_;

    static Config::RegisterID global_id_counter;
    Config::RegisterID id_;

    // Global starting index of this register.
    // This becomes important when a circuit contains
    // multiple quantum registers.
    std::size_t startIndex_;

public:
    explicit QuantumRegister(
        std::size_t size,
        const std::string& name = Config::DEFAULT_QUANTUM_REGISTER_NAME,
        std::size_t startIndex = 0
    );
    
    // Register Index
    Config::RegisterID id() const;
    
    // Number of qubits in the register.
    std::size_t size() const;

    // Register name.
    const std::string& name() const;

    // Starting global qubit index.
    std::size_t startIndex() const;

    // Access a qubit using register-local indexing.
    Qubit operator[](std::size_t index) const;
};

}

namespace Register
{
class ClassicalRegister
{
private:
    std::string name_;
    std::size_t size_;
    Config::RegisterID id_;
    std::size_t startIndex_;
    static Config::RegisterID global_id_counter;

public:
    explicit ClassicalRegister(
        std::size_t size,
        const std::string& name = Config::DEFAULT_CLASSICAL_REGISTER_NAME,
        std::size_t startIndex = 0);

    Config::RegisterID id() const;
    std::size_t size() const;
    const std::string& name() const;
    std::size_t startIndex() const;
};
}