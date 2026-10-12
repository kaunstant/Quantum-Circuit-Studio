#pragma once

#include "qubit.hpp"
#include <vector>
#include <cstddef>

namespace registers
{
class q_register
{
public:
    // Explicit constructor to initialize the register container with a fixed number of qubits
    explicit q_register(std::size_t size);  

    // Returns the number of qubits managed inside this register
    std::size_t size() const;

    // Subscript operators to easily index or look up individual qubits inside this group
    // qubit& operator[](std::size_t index);
    // const qubit& operator[](std::size_t index) const;

    // Concat assignment operator: merges another register's qubits array into this one
    q_register& operator+=(const q_register& other);

    // Friend concatenation operator: regC = regA + regB
    friend q_register operator+(q_register lhs, const q_register& rhs);

private:
    std::vector<qubit> m_qubits;
};
}