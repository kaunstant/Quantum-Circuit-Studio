#include "AlgorithmLibrary.hpp"
#include "ExampleSupport.hpp"
int main() { printExampleResult("Walsh-Hadamard transform", QuantumSimulator().run(Algorithms::walshHadamard(0, 3))); return 0; }