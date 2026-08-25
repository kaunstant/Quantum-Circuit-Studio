#pragma once

#include <cstddef>

// Represents Properties Associated with each Qubit.
class Qubit
{
private:
    std::size_t globalIndex_;
    std::size_t localIndex_;

    bool isRegister_;
    std::size_t registerId_;

public:
    explicit Qubit(std::size_t index);

    std::size_t index() const;
};
