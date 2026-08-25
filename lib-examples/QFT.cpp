#include <QCS>
#include "ExampleSupport.hpp"

int main() { printExampleResult("Quantum Fourier transform", QuantumSimulator().run(Algorithms::qft(0, 3))); return 0; }