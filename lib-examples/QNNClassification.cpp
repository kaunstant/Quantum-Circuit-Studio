#include <QCS>

#include <cmath>
#include <iostream>

int main()
{
    QNN::QNN model;
    model.setFeatureMap([](const std::vector<double>& features) { QuantumCircuit circuit(1); circuit.ry(0, features.at(0)); return circuit; });
    model.setAnsatz([](const std::vector<double>& parameters) { QuantumCircuit circuit(1); circuit.ry(0, parameters.at(0)); return circuit; });
    model.setParameters({0.35});
    model.setOutputMode(QNN::OutputMode::Probability);
    model.setOutputQubit(0);
    const std::vector<double> predictions = model.forwardBatch({{0.0}, {std::acos(-1.0)}});
    std::cout << "QNN classification: " << predictions.at(0) << ", " << predictions.at(1) << "\n";
    return 0;
}