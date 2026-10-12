#include "q_register.hpp"
#include <stdexcept>
#include <utility>

namespace registers
{

q_register::q_register(std::size_t size)
{
    // Instantiate 'size' number of blank qubit entities inside our array container
    m_qubits.resize(size);
}

std::size_t q_register::size() const {
    return m_qubits.size();
}

/*
qubit& q_register::operator[](std::size_t index) {
    if (index >= m_qubits.size()) {
        throw std::out_of_range("Register local subscript index out of bounds!");
    }
    return m_qubits[index];
}

const qubit& q_register::operator[](std::size_t index) const {
    if (index >= m_qubits.size()) {
        throw std::out_of_range("Register local subscript index out of bounds!");
    }
    return m_qubits[index];
}
*/

q_register& q_register::operator+=(const q_register& other) {
    const std::vector<qubit> appendedQubits(other.m_qubits);
    // Reserve memory upfront to avoid overhead reallocations
    m_qubits.reserve(m_qubits.size() + appendedQubits.size());
    
    // Copy the blank qubits from the incoming register into this container's array end
    m_qubits.insert(m_qubits.end(), appendedQubits.begin(), appendedQubits.end());
    
    return *this;
}

q_register operator+(q_register lhs, const q_register& rhs) {
    lhs += rhs;
    return lhs;
}

} // namespace registers
