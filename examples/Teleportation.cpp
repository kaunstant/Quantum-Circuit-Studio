#include "AlgorithmLibrary.hpp"
#include "ExampleSupport.hpp"
int main() { printExampleResult("Quantum teleportation", QuantumSimulator().run(Algorithms::teleportation())); return 0; }