#include "Qubit.hpp"

Qubit::Qubit(std::size_t index)
    : globalIndex_(index),
      localIndex_(index),
      isRegister_(false),
      registerId_(0)
{
}

std::size_t Qubit::index() const
{
    return globalIndex_;
}