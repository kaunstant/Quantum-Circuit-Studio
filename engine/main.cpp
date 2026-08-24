#include "QuantumCircuit.h"
#include "QuantumSimulator.h"

#include <iostream>
#include <vector>
#include <complex>
#include <bitset>
#include <iomanip>

#include <chrono>
#include <thread>

int N;
std::vector<std::complex<double>> state;

void PrintStateTable(const std::vector<std::complex<double>>& state, int N) {
    int totalStates = 1 << N;

    // Header
    std::cout << "\n+-------+------------+-------------------------+\n";
    std::cout << "| " << std::left << std::setw(5) << "Index" 
              << " | " << std::setw(10) << "Basis" 
              << " | " << std::setw(23) << "Amplitude (State)" 
              << " |\n";
    std::cout << "+-------+------------+-------------------------+\n";

    // Rows
    for (int i = 0; i < totalStates; ++i) {
        // Construct Basis string: e.g. |01>
        std::string basis = "|";
        for (int j = N - 1; j >= 0; --j) {
            basis += (((i >> j) & 1) ? '1' : '0');
        }
        basis += ">";

        // Convert complex number to string for uniform width alignment
        std::ostringstream ampStream;
        ampStream << state[i];

        std::cout << "| " << std::left << std::setw(5) << i 
                  << " | " << std::setw(10) << basis 
                  << " | " << std::setw(23) << ampStream.str() 
                  << " |\n";
    }

    std::cout << "+-------+------------+-------------------------+\n\n";
}

void InitializeCircuit() {

    std::cout << "Initialize Number of Qubits: ";
    std::cin >> N;
    std::cout << '\n';
    
    // Resize vector to 2^N elements after N is set
    state.assign(1 << N, 0.0);
    state[0] = 1.0; // Initialize to |00...0>

    // Display the initial state of the quantum system
    PrintStateTable(state, N);

}

void Output(std::chrono::duration<double, std::milli> duration) {

    // Display the final state of the quantum system after simulation
    PrintStateTable(state, N);

    std::cout << "Simulation Completed Successfully in " << duration.count() << " ms\n";

}

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    // Initialize Quantum Circuit : Default Initialization
    InitializeCircuit();

    // or set N and states manually

    /* 
    int N = 3;
    state.assign(1 << N, 0.0);
    state = {
        {1.0, 0.0},
        {0.0, 0.0},
        ....
        };

    PrintStateTable(state, N);
    */

    QuantumCircuit circuit(N);

    // Applying gates to the quantum circuit
    circuit.H(0);
    circuit.CX(1, 0);
    circuit.CX(2, 1);

    // Initialize Quantum Simulator
    QuantumSimulator simulator;

    // Pass the total number of amplitudes (1 << N) to match the simulator parameter
    simulator.run(circuit, state, 1 << N);

    // Displaying Final Amplitudes
    Output(simulator.duration);

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> p_duration = end - start;

    std::cout << "Program Completed Successfully in " << p_duration.count() << " ms\n";

    return 0;
}