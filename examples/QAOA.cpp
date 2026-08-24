#include "Variational.hpp"
#include "VariationalExampleSupport.hpp"

#include <cassert>
#include <iostream>

int main()
{
    // A two-vertex graph has one edge, so its maximum cut has value one.
    const Variational::QAOA qaoa(2, {{0, 1}});
    const std::vector<double> initialParameters = {0.8, 0.4};
    const Variational::OptimizationResult result = qaoa.minimize(
        initialParameters,
        Variational::CoordinateDescent(0.25, 80));

    printOptimizationResult("QAOA: maximum cut of a two-vertex graph", initialParameters, result, "Negative cut cost");
    assert(result.value < -0.5);
    std::cout << "Expected maximum cut value: 1\n";
    return 0;
}