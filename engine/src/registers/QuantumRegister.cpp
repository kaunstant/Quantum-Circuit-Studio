#include "QuantumRegister.hpp"

#include <stdexcept>


namespace Register
{
// =========================
// QuantumRegister
// =========================

Config::RegisterID Register::QuantumRegister::global_id_counter = 0;
Config::RegisterID ClassicalRegister::global_id_counter = 0;

QuantumRegister::QuantumRegister(
    std::size_t size, 
    const std::string& name, 
    std::size_t startIndex)
    : name_(name),
      size_(size),
      startIndex_(startIndex)
{
    if (size == 0)
        {
            throw std::invalid_argument(
                "QuantumRegister cannot contain zero qubits."
            );
        }

    this->id_ = global_id_counter;

    global_id_counter++;
}

Config::RegisterID QuantumRegister::id() const
{
    return id_;
}

std::size_t QuantumRegister::size() const
{
    return size_;
}


const std::string& QuantumRegister::name() const
{
    return name_;
}


std::size_t QuantumRegister::startIndex() const
{
    return startIndex_;
}


Qubit QuantumRegister::operator[](std::size_t index) const
{
    if (index >= size_)
    {
        throw std::out_of_range(
            "Qubit index is outside the quantum register."
        );
    }

    return Qubit(startIndex_ + index);
}

ClassicalRegister::ClassicalRegister(std::size_t size, const std::string& name, std::size_t startIndex)
    : name_(name), size_(size), id_(global_id_counter++), startIndex_(startIndex)
{
    if (size == 0)
        throw std::invalid_argument("ClassicalRegister cannot contain zero bits.");
}

Config::RegisterID ClassicalRegister::id() const { return id_; }
std::size_t ClassicalRegister::size() const { return size_; }
const std::string& ClassicalRegister::name() const { return name_; }
std::size_t ClassicalRegister::startIndex() const { return startIndex_; }

}