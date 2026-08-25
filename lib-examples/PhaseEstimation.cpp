#include <QCS>
#include "ExampleSupport.hpp"

int main() { Matrix phase = {{1.0, 0.0}, {0.0, std::complex<double>(0.0, 1.0)}}; printExampleResult("Phase estimation circuit", QuantumSimulator().run(Algorithms::phaseEstimation(phase, 2, 2))); return 0; }