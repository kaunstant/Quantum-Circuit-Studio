#include "GroverShor.hpp"
#include "ExampleSupport.hpp"

#include <iostream>

int main()
{
    Algorithms::GroverSearch search(Algorithms::GroverOracle(3, 5));
    const SimulationResult result = search.run();
    printExampleResult("Grover search: marked state |101>", result);
    std::cout << "Iterations: " << search.iterations() << "\n";
    return 0;
}