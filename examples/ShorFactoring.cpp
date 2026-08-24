#include "GroverShor.hpp"

#include <iostream>

int main()
{
    constexpr long long number = 15;
    const Algorithms::ShorFactors result = Algorithms::shorFactor(number);
    const Algorithms::Shor shor(number);
    const SimulationResult periodResult = QuantumSimulator().run(shor.periodFindingCircuit(4));
    std::cout << "Shor factoring via reusable subroutines\n";
    std::cout << "Classical preprocessing -> modular arithmetic -> order finding -> post-processing\n";
    if (!result.success || periodResult.state.empty()) return 1;
    std::cout << number << " = " << result.factorA << " * " << result.factorB << "\n";
    return 0;
}