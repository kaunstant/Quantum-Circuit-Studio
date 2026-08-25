#include <QCS>
#include "ExampleSupport.hpp"

int main() { printExampleResult("Deutsch-Jozsa algorithm", QuantumSimulator().run(Algorithms::deutschJozsa(3, [](QuantumCircuit& circuit) { circuit.cx(3, 0); }))); return 0; }