#pragma once

#include "ansatz.hpp"
#include "costfxn.hpp"

#include <cstddef>

namespace quantum::variational {

// Runs the variational quantum eigensolver loop using a Hamiltonian
void eigensolver(
    std::size_t maxIterations,
    double convergenceTolerance,
    ansatz& targetAnsatz,
    const quantum::costfxn::Hamiltonian& hamiltonian);

// Runs the combinatorial approximation loop using a graph cost function
void approximator(
    std::size_t maxIterations,
    ansatz& targetAnsatz,
    quantum::costfxn::CostFunction costFunction,
    double convergenceTolerance = 1e-6);

} // namespace quantum::variational
