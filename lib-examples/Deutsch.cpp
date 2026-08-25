#include <QCS>
#include "ExampleSupport.hpp"

int main() { printExampleResult("Deutsch algorithm", QuantumSimulator().run(Algorithms::deutsch([](QuantumCircuit& circuit) { circuit.cx(1, 0); }))); return 0; }