#include <QCS>
#include "ExampleSupport.hpp"

int main() { printExampleResult("Simon circuit scaffold", QuantumSimulator().run(Algorithms::simon({1, 0, 1}))); return 0; }