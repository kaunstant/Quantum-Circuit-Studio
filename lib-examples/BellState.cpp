#include <QCS>
#include "ExampleSupport.hpp"

int main() { QuantumCircuit circuit(2); circuit.h(0); circuit.cx(1, 0); printExampleResult("Bell state: |00> + |11>", QuantumSimulator().run(circuit)); return 0; }