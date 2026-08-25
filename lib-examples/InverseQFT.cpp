#include <QCS>
#include "ExampleSupport.hpp"

int main() { printExampleResult("Inverse quantum Fourier transform", QuantumSimulator().run(Algorithms::inverseQft(0, 3))); return 0; }