#include <QCS>
#include "ExampleSupport.hpp"

int main() { QuantumCircuit circuit(3); circuit.h(0); circuit.cx(1, 0); circuit.cx(2, 1); printExampleResult("GHZ state: |000> + |111>", QuantumSimulator().run(circuit)); return 0; }