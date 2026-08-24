#include "QuantumSimulator.hpp"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>

namespace
{
using Complex=std::complex<double>;

void applyMatrix(std::vector<Complex>& state,const Matrix& matrix,const std::vector<int>& qubits,int control=-1)
{
    const std::size_t dimension=std::size_t{1}<<qubits.size(); auto original=state;
    for(std::size_t basis=0;basis<state.size();++basis)
    {
        if(control>=0 && !(basis&(std::size_t{1}<<control))) continue;
        std::size_t column=0; for(std::size_t bit=0;bit<qubits.size();++bit) if(basis&(std::size_t{1}<<qubits[bit])) column|=std::size_t{1}<<bit;
        Complex value=0.0;
        for(std::size_t row=0;row<dimension;++row) { std::size_t source=basis; for(std::size_t bit=0;bit<qubits.size();++bit) source=(source&~(std::size_t{1}<<qubits[bit]))|(((row>>bit)&1)<<qubits[bit]); value+=matrix[row][column]*original[source]; }
        state[basis]=value;
    }
}

void collapse(std::vector<Complex>& state,int qubit,int result)
{
    double norm=0.0; for(std::size_t i=0;i<state.size();++i) if(static_cast<int>((i>>qubit)&1)!=result) state[i]=0.0; else norm+=std::norm(state[i]);
    if(norm>0.0) for(auto& value:state) value/=std::sqrt(norm);
}
}

std::vector<double> QuantumSimulator::probabilities(const std::vector<Complex>& state) { std::vector<double> result; for(const auto& value:state) result.push_back(std::norm(value)); return result; }

void QuantumSimulator::run(const QuantumCircuit& circuit,std::vector<Complex>& state,int N)
{
    if(N!=(1<<circuit.numQubits()) || state.size()!=static_cast<std::size_t>(N)) throw std::invalid_argument("State size does not match circuit qubit count.");
    duration={}; const Complex I(0.0,1.0); std::mt19937 generator(42);
    for(const auto& gate:circuit.getGates())
    {
        if (!gate.parameter.empty()) throw std::invalid_argument("Circuit contains an unbound parameter: " + gate.parameter);
        const auto start=std::chrono::high_resolution_clock::now(); const int target=gate.qubit1 >= 0 ? 1<<gate.qubit1 : 0;
        if(gate.operation==OperationType::Matrix) applyMatrix(state,gate.matrix,gate.qubits);
        else if(gate.operation==OperationType::ControlledMatrix) applyMatrix(state,gate.matrix,gate.qubits,gate.qubit1);
        else if(gate.operation==OperationType::MultiControlledX)
            for(int i=0;i<N;++i) { bool enabled=true; for(int control:gate.controls) enabled=enabled && (i&(1<<control)); if(enabled && !(i&target)) std::swap(state[i],state[i|target]); }
        else if(gate.type==GateType::CX||gate.type==GateType::CY||gate.type==GateType::CZ)
        {
            const int control=1<<gate.qubit2;
            for(int i=0;i<N;++i) if((i&control)&&!(i&target)) { const int j=i|target; Complex a=state[i],b=state[j]; if(gate.type==GateType::CX) { state[i]=b; state[j]=a; } else if(gate.type==GateType::CY) { state[i]=-I*b; state[j]=I*a; } else if(gate.theta!=0.0) state[j]*=std::exp(I*gate.theta); else { state[i]=-state[i]; state[j]=-state[j]; } }
        }
        else if(gate.operation==OperationType::Measurement)
        {
            const auto p=probabilities(state); double sample=std::uniform_real_distribution<double>(0.0,1.0)(generator), cumulative=0.0; int result=0;
            for(std::size_t i=0;i<p.size();++i) { cumulative+=p[i]; if(cumulative>=sample) { result=static_cast<int>((i>>gate.qubit1)&1); break; } } collapse(state,gate.qubit1,result);
        }
        else if(gate.operation==OperationType::Gate)
        {
            if(gate.type==GateType::X||gate.type==GateType::Y||gate.type==GateType::H||gate.type==GateType::RX||gate.type==GateType::RY)
            {
                const double c=gate.type==GateType::H?1/std::sqrt(2.0):std::cos(gate.theta/2), s=gate.type==GateType::H?1/std::sqrt(2.0):std::sin(gate.theta/2);
                for(int i=0;i<N;++i) if(!(i&target)) { const int j=i|target; Complex a=state[i],b=state[j]; if(gate.type==GateType::X){state[i]=b;state[j]=a;} else if(gate.type==GateType::Y){state[i]=-I*b;state[j]=I*a;} else if(gate.type==GateType::RY){state[i]=c*a-s*b;state[j]=s*a+c*b;} else if(gate.type==GateType::H){state[i]=c*(a+b);state[j]=c*(a-b);} else {state[i]=c*a-I*s*b;state[j]=-I*s*a+c*b;} }
            }
            else if(gate.type==GateType::Z||gate.type==GateType::T||gate.type==GateType::RZ)
                for(int i=0;i<N;++i) if(i&target) state[i]*=gate.type==GateType::Z?Complex(-1):gate.type==GateType::T?std::exp(I*std::acos(-1.0)/4.0):std::exp(I*gate.theta/2.0); else if(gate.type==GateType::RZ) state[i]*=std::exp(-I*gate.theta/2.0);
            else if(gate.type==GateType::SWAP) for(int i=0;i<N;++i) if(!(i&(1<<gate.qubit1))&&(i&(1<<gate.qubit2))) std::swap(state[i],state[(i|(1<<gate.qubit1))&~(1<<gate.qubit2)]);
            else if(gate.type==GateType::CCX) for(int i=0;i<N;++i) if((i&(1<<gate.qubit2))&&(i&(1<<gate.qubit3))&&!(i&target)) std::swap(state[i],state[i|target]);
        }
        duration+=std::chrono::high_resolution_clock::now()-start;
    }
}

SimulationResult QuantumSimulator::run(const QuantumCircuit& circuit)
{
    SimulationResult result; result.state.assign(std::size_t{1}<<circuit.numQubits(),Complex(0.0,0.0)); result.state[0]=1.0; result.classicalBits.assign(Config::DEFAULT_MAX_CLASSICAL_BITS,0);
    run(circuit,result.state,static_cast<int>(result.state.size()));
    for(const auto& gate:circuit.getGates()) if(gate.operation==OperationType::Measurement) { const auto p=probabilities(result.state); auto selected=std::distance(p.begin(),std::max_element(p.begin(),p.end())); result.classicalBits[gate.classicalBit]=static_cast<int>((selected>>gate.qubit1)&1); }
    return result;
}