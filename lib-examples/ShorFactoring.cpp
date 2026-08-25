#include <QCS>

#include <iostream>

int main()
{
    constexpr long long number = 15;
    const Algorithms::ShorFactors result = Algorithms::shorFactor(number);
    const Algorithms::Shor shor(number);
    const SimulationResult periodResult = QuantumSimulator().run(shor.periodFindingCircuit(4));
    if (!result.success || periodResult.state.empty()) return 1;
    std::cout << number << " = " << result.factorA << " * " << result.factorB << "\n";
    return 0;
}