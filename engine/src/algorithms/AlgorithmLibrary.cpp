#include "AlgorithmLibrary.hpp"

#include <cmath>
#include <stdexcept>

namespace
{
Matrix multiply(const Matrix& left, const Matrix& right)
{
    const std::size_t size=left.size(); Matrix result(size,std::vector<std::complex<double>>(size));
    for(std::size_t row=0;row<size;++row) for(std::size_t column=0;column<size;++column) for(std::size_t k=0;k<size;++k) result[row][column]+=left[row][k]*right[k][column];
    return result;
}

Matrix singlePhase(double angle)
{
    return {{1.0,0.0},{0.0,std::exp(std::complex<double>(0.0,angle))}};
}

void validateRange(int first,int count)
{
    if(first<0 || count<=0) throw std::invalid_argument("Algorithm qubit range is invalid.");
}
}

namespace Algorithms
{
// =============================
// Transformations
// =============================

QuantumCircuit qft(int first,int count)
{
    validateRange(first,count); QuantumCircuit circuit(first+count); const double pi=std::acos(-1.0);
    for(int target=first;target<first+count;++target) { circuit.h(target); for(int control=target+1;control<first+count;++control) circuit.controlledPhase(control,target,pi/std::pow(2.0,control-target)); }
    for(int offset=0;offset<count/2;++offset) circuit.swap(first+offset,first+count-1-offset);
    return circuit;
}

QuantumCircuit inverseQft(int first,int count)
{
    QuantumCircuit circuit(first+count); for(int offset=0;offset<count/2;++offset) circuit.swap(first+offset,first+count-1-offset);
    const double pi=std::acos(-1.0); for(int target=first+count-1;target>=first;--target) { for(int control=target+1;control<first+count;++control) circuit.controlledPhase(control,target,-pi/std::pow(2.0,control-target)); circuit.h(target); }
    return circuit;
}

QuantumCircuit walshHadamard(int first,int count)
{
    validateRange(first,count); QuantumCircuit circuit(first+count); for(int q=first;q<first+count;++q) circuit.h(q); return circuit;
}

// =============================
// Algorithm primitives
// =============================

QuantumCircuit phaseEstimation(const Matrix& unitary,int countingQubits,int firstTarget)
{
    if(countingQubits<=0) throw std::invalid_argument("Phase estimation needs counting qubits.");
    QuantumCircuit circuit=QuantumCircuit(std::max(countingQubits,firstTarget+1)); for(int q=0;q<countingQubits;++q) circuit.h(q);
    Matrix power=unitary; for(int counting=0;counting<countingQubits;++counting) { for(int repetition=0;repetition<(1<<counting);++repetition) circuit.controlled(power,counting,{firstTarget}); power=multiply(power,power); }
    const double pi=std::acos(-1.0); for(int target=countingQubits-1;target>=0;--target) { for(int control=target+1;control<countingQubits;++control) circuit.controlledPhase(control,target,-pi/std::pow(2.0,control-target)); circuit.h(target); }
    for(int offset=0;offset<countingQubits/2;++offset) circuit.swap(offset,countingQubits-1-offset); return circuit;
}

QuantumCircuit amplitudeAmplification(int qubitCount,const CircuitBuilder& oracle)
{
    QuantumCircuit circuit(qubitCount); for(int q=0;q<qubitCount;++q) circuit.h(q); oracle(circuit);
    for(int q=0;q<qubitCount;++q) circuit.h(q); for(int q=0;q<qubitCount;++q) circuit.x(q);
    circuit.h(qubitCount-1); std::vector<int> controls; for(int q=0;q<qubitCount-1;++q) controls.push_back(q); circuit.multiControlledX(controls,qubitCount-1); circuit.h(qubitCount-1);
    for(int q=0;q<qubitCount;++q) circuit.x(q); for(int q=0;q<qubitCount;++q) circuit.h(q); return circuit;
}

// =============================
// Communication and query algorithms
// =============================

QuantumCircuit teleportation()
{
    QuantumCircuit circuit(3); circuit.h(1); circuit.cx(2,1); circuit.cx(1,0); circuit.h(1); circuit.cx(0,1); circuit.cz(0,2); return circuit;
}

QuantumCircuit superdenseCoding(int bit0,int bit1)
{
    QuantumCircuit circuit(2); circuit.h(1); circuit.cx(0,1); if(bit1) circuit.z(1); if(bit0) circuit.x(1); circuit.cx(0,1); circuit.h(1); return circuit;
}

QuantumCircuit deutsch(const CircuitBuilder& oracle)
{
    QuantumCircuit circuit(2); circuit.x(0); circuit.h(0); circuit.h(1); oracle(circuit); circuit.h(1); return circuit;
}

QuantumCircuit deutschJozsa(int inputQubits,const CircuitBuilder& oracle)
{
    QuantumCircuit circuit(inputQubits+1); circuit.x(inputQubits); for(int q=0;q<=inputQubits;++q) circuit.h(q); oracle(circuit); for(int q=0;q<inputQubits;++q) circuit.h(q); return circuit;
}

QuantumCircuit bernsteinVazirani(const std::vector<int>& secret)
{
    const int output=static_cast<int>(secret.size()); QuantumCircuit circuit(output+1); circuit.x(output); for(int q=0;q<=output;++q) circuit.h(q); for(int q=0;q<output;++q) if(secret[q]) circuit.cx(output,q); for(int q=0;q<output;++q) circuit.h(q); return circuit;
}

QuantumCircuit simon(const std::vector<int>& secret)
{
    const int width=static_cast<int>(secret.size()); QuantumCircuit circuit(width*2); for(int q=0;q<width;++q) circuit.h(q); for(int q=0;q<width;++q) circuit.cx(q+width,q); for(int q=0;q<width;++q) if(secret[q]) circuit.cx(q,q+width); for(int q=0;q<width;++q) circuit.h(q); return circuit;
}
}