#include "AlgorithmLibrary.hpp"
#include "ExampleSupport.hpp"
int main() { printExampleResult("Bernstein-Vazirani algorithm", QuantumSimulator().run(Algorithms::bernsteinVazirani({1, 0, 1}))); return 0; }