#include "QuantumSimulator.h"
#include "QuantumCircuit.h"

#include <iostream>
#include <cmath>
#include <chrono>

void QuantumSimulator::run(
    const QuantumCircuit &circuit,
    std::vector<std::complex<double>> &state,
    int N)
{
    std::cout << "\nRunning Quantum Simulation...\n\n";

    const auto &gates = circuit.getGates();
    const std::complex<double> I(0.0, 1.0);
    const std::complex<double> exp_i_pi_4(1.0 / std::sqrt(2.0), 1.0 / std::sqrt(2.0));

    for (const auto &gate : gates)
    {
        switch (gate.type)
        {
        case GateType::X:
        {
            std::cout << "Executing X(" << gate.qubit1 << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int targetBit = 1 << gate.qubit1;
            for (int i = 0; i < N; i++)
            {
                if ((i & targetBit) == 0)
                {
                    int j = i | targetBit;
                    std::swap(state[i], state[j]);
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::Y:
        {
            std::cout << "Executing Y(" << gate.qubit1 << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int targetBit = 1 << gate.qubit1;
            for (int i = 0; i < N; i++)
            {
                if ((i & targetBit) == 0)
                {
                    int j = i | targetBit;
                    std::complex<double> v0 = state[i];
                    std::complex<double> v1 = state[j];
                    state[i] = -I * v1;
                    state[j] = I * v0;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::Z:
        {
            std::cout << "Executing Z(" << gate.qubit1 << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int targetBit = 1 << gate.qubit1;
            for (int i = 0; i < N; i++)
            {
                if ((i & targetBit) != 0)
                {
                    state[i] = -state[i];
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::H:
        {
            std::cout << "Executing H(" << gate.qubit1 << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            double invSqrt2 = 1.0 / std::sqrt(2.0);
            int targetBit = 1 << gate.qubit1;
            for (int i = 0; i < N; i++)
            {
                if ((i & targetBit) == 0)
                {
                    int j = i | targetBit;
                    std::complex<double> v0 = state[i];
                    std::complex<double> v1 = state[j];
                    state[i] = invSqrt2 * (v0 + v1);
                    state[j] = invSqrt2 * (v0 - v1);
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::T:
        {
            std::cout << "Executing T(" << gate.qubit1 << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int targetBit = 1 << gate.qubit1;
            for (int i = 0; i < N; i++)
            {
                if ((i & targetBit) != 0)
                {
                    state[i] *= exp_i_pi_4;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::RX:
        {
            std::cout << "Executing RX(" << gate.qubit1 << ", " << gate.theta << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int targetBit = 1 << gate.qubit1;
            double cosVal = std::cos(gate.theta / 2.0);
            double sinVal = std::sin(gate.theta / 2.0);

            for (int i = 0; i < N; i++)
            {
                if ((i & targetBit) == 0)
                {
                    int j = i | targetBit;
                    std::complex<double> v0 = state[i];
                    std::complex<double> v1 = state[j];
                    state[i] = cosVal * v0 - I * sinVal * v1;
                    state[j] = -I * sinVal * v0 + cosVal * v1;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::RY:
        {
            std::cout << "Executing RY(" << gate.qubit1 << ", " << gate.theta << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int targetBit = 1 << gate.qubit1;
            double cosVal = std::cos(gate.theta / 2.0);
            double sinVal = std::sin(gate.theta / 2.0);

            for (int i = 0; i < N; i++)
            {
                if ((i & targetBit) == 0)
                {
                    int j = i | targetBit;
                    std::complex<double> v0 = state[i];
                    std::complex<double> v1 = state[j];
                    state[i] = cosVal * v0 - sinVal * v1;
                    state[j] = sinVal * v0 + cosVal * v1;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::RZ:
        {
            std::cout << "Executing RZ(" << gate.qubit1 << ", " << gate.theta << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int targetBit = 1 << gate.qubit1;
            std::complex<double> phase0 = std::exp(-I * (gate.theta / 2.0));
            std::complex<double> phase1 = std::exp(I * (gate.theta / 2.0));

            for (int i = 0; i < N; i++)
            {
                if ((i & targetBit) == 0)
                {
                    state[i] *= phase0;
                }
                else
                {
                    state[i] *= phase1;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::CX:
        {
            std::cout << "Executing CX(" << gate.qubit1 << ", " << gate.qubit2 << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int targetBit = 1 << gate.qubit1;
            int ctrlBit = 1 << gate.qubit2;

            for (int i = 0; i < N; i++)
            {
                if (((i & ctrlBit) != 0) && ((i & targetBit) == 0))
                {
                    int j = i | targetBit;
                    std::swap(state[i], state[j]);
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::CY:
        {
            std::cout << "Executing CY(" << gate.qubit1 << ", " << gate.qubit2 << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int targetBit = 1 << gate.qubit1;
            int ctrlBit = 1 << gate.qubit2;

            for (int i = 0; i < N; i++)
            {
                if (((i & ctrlBit) != 0) && ((i & targetBit) == 0))
                {
                    int j = i | targetBit;
                    std::complex<double> v0 = state[i];
                    std::complex<double> v1 = state[j];
                    state[i] = -I * v1;
                    state[j] = I * v0;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::CZ:
        {
            std::cout << "Executing CZ(" << gate.qubit1 << ", " << gate.qubit2 << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int targetBit = 1 << gate.qubit1;
            int ctrlBit = 1 << gate.qubit2;

            for (int i = 0; i < N; i++)
            {
                if (((i & ctrlBit) != 0) && ((i & targetBit) != 0))
                {
                    state[i] = -state[i];
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::SWAP:
        {
            std::cout << "Executing SWAP(" << gate.qubit1 << ", " << gate.qubit2 << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int bit1 = 1 << gate.qubit1;
            int bit2 = 1 << gate.qubit2;

            for (int i = 0; i < N; i++)
            {
                bool b1 = (i & bit1) != 0;
                bool b2 = (i & bit2) != 0;

                if (!b1 && b2)
                {
                    int j = (i | bit1) & (~bit2);
                    std::swap(state[i], state[j]);
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        case GateType::CCX:
        {
            std::cout << "Executing CCX(" << gate.qubit1 << ", " << gate.qubit2 << ", " << gate.qubit3 << ")\n";
            auto start = std::chrono::high_resolution_clock::now();

            int targetBit = 1 << gate.qubit1;
            int ctrl1 = 1 << gate.qubit2;
            int ctrl2 = 1 << gate.qubit3;

            for (int i = 0; i < N; i++)
            {
                if (((i & ctrl1) != 0) && ((i & ctrl2) != 0) && ((i & targetBit) == 0))
                {
                    int j = i | targetBit;
                    std::swap(state[i], state[j]);
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            duration += end - start;
            break;
        }

        default:
            break;
        }
    }
}