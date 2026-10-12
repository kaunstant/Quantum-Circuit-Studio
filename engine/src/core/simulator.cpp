#include "simulator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>

namespace quantum {
namespace {

// Keep numeric tolerances and matrix kernels local to the simulator implementation.
constexpr double kTolerance = 1e-12;
constexpr double kPi = 3.141592653589793238462643383279502884;
using SingleQubitMatrix = std::array<Complex, 4>;

bool hasDistinctWires(const gate& operation) {
    for (std::size_t i = 0; i < operation.targets.size(); ++i) {
        for (std::size_t j = i + 1; j < operation.targets.size(); ++j) {
            if (operation.targets[i] == operation.targets[j]) {
                return false;
            }
        }
        for (const int control : operation.controls) {
            if (operation.targets[i] == control) {
                return false;
            }
        }
    }
    for (std::size_t i = 0; i < operation.controls.size(); ++i) {
        for (std::size_t j = i + 1; j < operation.controls.size(); ++j) {
            if (operation.controls[i] == operation.controls[j]) {
                return false;
            }
        }
    }
    return true;
}

bool isUnitary2x2(const Matrix2D& matrix) {
    if (matrix.size() != 2) {
        return false;
    }
    for (const auto& row : matrix) {
        if (row.size() != 2) {
            return false;
        }
    }
    for (const auto& row : matrix) {
        for (const Complex value : row) {
            if (!std::isfinite(value.real()) || !std::isfinite(value.imag())) {
                return false;
            }
        }
    }

    for (std::size_t row = 0; row < 2; ++row) {
        for (std::size_t otherRow = 0; otherRow < 2; ++otherRow) {
            Complex innerProduct{0.0, 0.0};
            for (std::size_t column = 0; column < 2; ++column) {
                innerProduct += matrix[row][column] * std::conj(matrix[otherRow][column]);
            }
            const Complex expected = row == otherRow ? Complex{1.0, 0.0} : Complex{0.0, 0.0};
            if (std::abs(innerProduct - expected) > kTolerance) {
                return false;
            }
        }
    }
    return true;
}

bool isUnitary(const Matrix2D& matrix, std::size_t dimension) {
    if (matrix.size() != dimension) {
        return false;
    }
    for (const auto& row : matrix) {
        if (row.size() != dimension) {
            return false;
        }
        for (const Complex value : row) {
            if (!std::isfinite(value.real()) || !std::isfinite(value.imag())) {
                return false;
            }
        }
    }

    // Unitarity requires every pair of rows to have the identity inner product.
    for (std::size_t row = 0; row < dimension; ++row) {
        for (std::size_t otherRow = 0; otherRow < dimension; ++otherRow) {
            Complex innerProduct{0.0, 0.0};
            for (std::size_t column = 0; column < dimension; ++column) {
                innerProduct += matrix[row][column] * std::conj(matrix[otherRow][column]);
            }
            const Complex expected = row == otherRow ? Complex{1.0, 0.0} : Complex{0.0, 0.0};
            if (std::abs(innerProduct - expected) > kTolerance) {
                return false;
            }
        }
    }
    return true;
}

void applySingleQubitMatrix(
    std::vector<Complex>& state,
    int target,
    const std::vector<int>& controls,
    const SingleQubitMatrix& matrix)
{
    // Each matching pair is read before either amplitude is overwritten.
    const std::size_t targetMask = std::size_t{1} << static_cast<unsigned int>(target);
    std::size_t controlMask = 0;
    for (const int control : controls) {
        controlMask |= std::size_t{1} << static_cast<unsigned int>(control);
    }

    for (std::size_t basis = 0; basis < state.size(); ++basis) {
        if ((basis & targetMask) != 0 || (basis & controlMask) != controlMask) {
            continue;
        }
        const std::size_t pairedBasis = basis | targetMask;
        const Complex zero = state[basis];
        const Complex one = state[pairedBasis];
        state[basis] = matrix[0] * zero + matrix[1] * one;
        state[pairedBasis] = matrix[2] * zero + matrix[3] * one;
    }
}

void applyConditionalPhase(
    std::vector<Complex>& state,
    int target,
    const std::vector<int>& controls,
    const Complex& phase)
{
    const std::size_t targetMask = std::size_t{1} << static_cast<unsigned int>(target);
    std::size_t controlMask = 0;
    for (const int control : controls) {
        controlMask |= std::size_t{1} << static_cast<unsigned int>(control);
    }
    for (std::size_t basis = 0; basis < state.size(); ++basis) {
        if ((basis & targetMask) != 0 && (basis & controlMask) == controlMask) {
            state[basis] *= phase;
        }
    }
}

void applyTwoWirePhase(
    std::vector<Complex>& state,
    int firstWire,
    int secondWire,
    double angle)
{
    const std::size_t firstMask = std::size_t{1} << static_cast<unsigned int>(firstWire);
    const std::size_t secondMask = std::size_t{1} << static_cast<unsigned int>(secondWire);
    const Complex phase = std::exp(Complex{0.0, angle});
    for (std::size_t basis = 0; basis < state.size(); ++basis) {
        if ((basis & firstMask) != 0 && (basis & secondMask) != 0) {
            state[basis] *= phase;
        }
    }
}

void applySwap(
    std::vector<Complex>& state,
    int firstWire,
    int secondWire,
    const std::vector<int>& controls = {})
{
    const std::size_t firstMask = std::size_t{1} << static_cast<unsigned int>(firstWire);
    const std::size_t secondMask = std::size_t{1} << static_cast<unsigned int>(secondWire);
    std::size_t controlMask = 0;
    for (const int control : controls) {
        controlMask |= std::size_t{1} << static_cast<unsigned int>(control);
    }
    for (std::size_t basis = 0; basis < state.size(); ++basis) {
        if ((basis & firstMask) == 0 && (basis & secondMask) != 0 &&
            (basis & controlMask) == controlMask) {
            const std::size_t pairedBasis = basis ^ firstMask ^ secondMask;
            std::swap(state[basis], state[pairedBasis]);
        }
    }
}

SingleQubitMatrix namedMatrix(const std::string& name, double parameter = 0.0) {
    const Complex imaginary{0.0, 1.0};
    if (name == "X") {
        return {0.0, 1.0, 1.0, 0.0};
    }
    if (name == "Y") {
        return {0.0, -imaginary, imaginary, 0.0};
    }
    if (name == "H") {
        const double scale = 1.0 / std::sqrt(2.0);
        return {scale, scale, scale, -scale};
    }
    if (name == "Rx") {
        const double cosine = std::cos(parameter / 2.0);
        const Complex offDiagonal = -imaginary * std::sin(parameter / 2.0);
        return {cosine, offDiagonal, offDiagonal, cosine};
    }
    if (name == "Ry") {
        const double cosine = std::cos(parameter / 2.0);
        const double sine = std::sin(parameter / 2.0);
        return {cosine, -sine, sine, cosine};
    }
    throw std::logic_error("Requested gate does not have a dense single-qubit matrix");
}

void applyNamedGate(
    std::vector<Complex>& state,
    const gate& operation,
    const std::string& name,
    const std::vector<int>& controls)
{
    const int target = operation.targets.front();
    if (name == "Z") {
        applyConditionalPhase(state, target, controls, Complex{-1.0, 0.0});
    } else if (name == "T") {
        applyConditionalPhase(state, target, controls, std::exp(Complex{0.0, kPi / 4.0}));
    } else if (name == "Rz") {
        const double angle = operation.parameters[0] / 2.0;
        const Complex phaseZero = std::exp(Complex{0.0, -angle});
        const Complex phaseOne = std::exp(Complex{0.0, angle});
        std::size_t controlMask = 0;
        for (const int control : controls) {
            controlMask |= std::size_t{1} << static_cast<unsigned int>(control);
        }
        const std::size_t targetMask = std::size_t{1} << static_cast<unsigned int>(target);
        for (std::size_t basis = 0; basis < state.size(); ++basis) {
            if ((basis & controlMask) == controlMask) {
                state[basis] *= (basis & targetMask) != 0 ? phaseOne : phaseZero;
            }
        }
    } else if (name == "SWAP") {
        applySwap(state, operation.targets[0], operation.targets[1], controls);
    } else {
        double parameter = 0.0;
        if (name == "Rx" || name == "Ry") {
            parameter = operation.parameters[0];
        }
        applySingleQubitMatrix(state, target, controls, namedMatrix(name, parameter));
    }
}

void applyExplicitMatrix(
    std::vector<Complex>& state,
    int target,
    const std::vector<int>& controls,
    const Matrix2D& matrix)
{
    const SingleQubitMatrix values{
        matrix[0][0], matrix[0][1], matrix[1][0], matrix[1][1]};
    applySingleQubitMatrix(state, target, controls, values);
}

void applyRegisterMatrix(
    std::vector<Complex>& state,
    std::size_t startWire,
    std::size_t registerWidth,
    const Matrix2D& matrix,
    const std::vector<int>& controls)
{
    const std::size_t dimension = std::size_t{1} << registerWidth;
    const std::size_t registerMask = (dimension - 1) << startWire;
    std::size_t controlMask = 0;
    for (const int control : controls) {
        controlMask |= std::size_t{1} << static_cast<unsigned int>(control);
    }
    std::vector<std::size_t> basisOffsets(dimension);
    std::vector<Complex> input(dimension);
    std::vector<Complex> output(dimension);

    for (std::size_t localBasis = 0; localBasis < dimension; ++localBasis) {
        basisOffsets[localBasis] = localBasis << startWire;
    }

    // Transform one register-sized amplitude block at a time, preserving all other wires.
    for (std::size_t base = 0; base < state.size(); ++base) {
        if ((base & registerMask) != 0 || (base & controlMask) != controlMask) {
            continue;
        }

        for (std::size_t column = 0; column < dimension; ++column) {
            input[column] = state[base | basisOffsets[column]];
        }
        for (std::size_t row = 0; row < dimension; ++row) {
            Complex amplitude{0.0, 0.0};
            for (std::size_t column = 0; column < dimension; ++column) {
                amplitude += matrix[row][column] * input[column];
            }
            output[row] = amplitude;
        }
        for (std::size_t row = 0; row < dimension; ++row) {
            state[base | basisOffsets[row]] = output[row];
        }
    }
}

void applyStandardGate(std::vector<Complex>& state, const gate& operation) {
    const std::string& name = operation.name;
    if (name == "CX" || name == "CCX") {
        applySingleQubitMatrix(state, operation.targets[0], operation.controls, namedMatrix("X"));
    } else if (name == "CY") {
        applySingleQubitMatrix(state, operation.targets[0], operation.controls, namedMatrix("Y"));
    } else if (name == "CZ") {
        applyConditionalPhase(state, operation.targets[0], operation.controls, Complex{-1.0, 0.0});
    } else if (name == "SWAP") {
        applySwap(state, operation.targets[0], operation.targets[1]);
    } else {
        applyNamedGate(state, operation, name, {});
    }
}

void applyControlledGate(std::vector<Complex>& state, const gate& operation) {
    applyExplicitMatrix(state, operation.targets[0], operation.controls, operation.explicitMatrix);
}

void applyRegisterFourier(
    std::vector<Complex>& state,
    std::size_t startWire,
    std::size_t width,
    bool inverse)
{
    if (inverse) {
        for (std::size_t left = 0; left < width / 2; ++left) {
            const std::size_t right = width - left - 1;
            applySwap(state, static_cast<int>(startWire + left), static_cast<int>(startWire + right));
        }
        const SingleQubitMatrix hadamard = namedMatrix("H");
        for (std::size_t target = 0; target < width; ++target) {
            for (std::size_t control = 0; control < target; ++control) {
                const std::size_t distance = target - control;
                const double angle = -kPi / static_cast<double>(std::size_t{1} << distance);
                applyTwoWirePhase(
                    state,
                    static_cast<int>(startWire + target),
                    static_cast<int>(startWire + control),
                    angle);
            }
            applySingleQubitMatrix(state, static_cast<int>(startWire + target), {}, hadamard);
        }
        return;
    }

    const SingleQubitMatrix hadamard = namedMatrix("H");
    for (std::size_t target = width; target-- > 0;) {
        applySingleQubitMatrix(state, static_cast<int>(startWire + target), {}, hadamard);
        for (std::size_t control = target; control-- > 0;) {
            const std::size_t distance = target - control;
            const double angle = kPi / static_cast<double>(std::size_t{1} << distance);
            applyTwoWirePhase(
                state,
                static_cast<int>(startWire + target),
                static_cast<int>(startWire + control),
                angle);
        }
    }
    for (std::size_t left = 0; left < width / 2; ++left) {
        const std::size_t right = width - left - 1;
        applySwap(state, static_cast<int>(startWire + left), static_cast<int>(startWire + right));
    }
}

void applyWalshHadamard(std::vector<Complex>& state, std::size_t startWire, std::size_t width) {
    const SingleQubitMatrix hadamard = namedMatrix("H");
    for (std::size_t wire = 0; wire < width; ++wire) {
        applySingleQubitMatrix(state, static_cast<int>(startWire + wire), {}, hadamard);
    }
}

void applyDiffusion(
    std::vector<Complex>& state,
    std::size_t startWire,
    std::size_t width,
    std::size_t iterations)
{
    const std::size_t dimension = std::size_t{1} << width;
    const std::size_t registerMask = (dimension - 1) << startWire;
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        for (std::size_t base = 0; base < state.size(); ++base) {
            if ((base & registerMask) != 0) {
                continue;
            }
            Complex sum{0.0, 0.0};
            for (std::size_t offset = 0; offset < dimension; ++offset) {
                sum += state[base | (offset << startWire)];
            }
            const Complex twiceMean = (2.0 / static_cast<double>(dimension)) * sum;
            for (std::size_t offset = 0; offset < dimension; ++offset) {
                Complex& amplitude = state[base | (offset << startWire)];
                amplitude = twiceMean - amplitude;
            }
        }
    }
}

double oneProbability(const std::vector<Complex>& state, int wire) {
    const std::size_t mask = std::size_t{1} << static_cast<unsigned int>(wire);
    double one = 0.0;
    double total = 0.0;
    for (std::size_t basis = 0; basis < state.size(); ++basis) {
        const double probability = std::norm(state[basis]);
        total += probability;
        if ((basis & mask) != 0) {
            one += probability;
        }
    }
    if (!std::isfinite(total) || total <= 0.0) {
        throw std::runtime_error("Cannot sample a state vector with zero or invalid norm");
    }
    return std::clamp(one / total, 0.0, 1.0);
}

std::vector<double> registerProbabilities(
    const std::vector<Complex>& state,
    const std::vector<int>& targets)
{
    if (targets.empty() || targets.size() >= std::numeric_limits<std::size_t>::digits) {
        throw std::length_error("Register is too large to sample");
    }
    const std::size_t dimension = std::size_t{1} << targets.size();
    std::vector<double> probabilities(dimension, 0.0);
    for (std::size_t basis = 0; basis < state.size(); ++basis) {
        std::size_t localBasis = 0;
        for (std::size_t localWire = 0; localWire < targets.size(); ++localWire) {
            const std::size_t mask =
                std::size_t{1} << static_cast<unsigned int>(targets[localWire]);
            if ((basis & mask) != 0) {
                localBasis |= std::size_t{1} << localWire;
            }
        }
        probabilities[localBasis] += std::norm(state[basis]);
    }
    double total = 0.0;
    for (const double probability : probabilities) {
        total += probability;
    }
    if (!std::isfinite(total) || total <= 0.0) {
        throw std::runtime_error("Cannot sample a state vector with zero or invalid norm");
    }
    for (double& probability : probabilities) {
        probability /= total;
    }
    return probabilities;
}

std::string registerBitString(std::size_t value, std::size_t width) {
    std::string bits(width, '0');
    for (std::size_t bit = 0; bit < width; ++bit) {
        if ((value & (std::size_t{1} << bit)) != 0) {
            bits[width - bit - 1] = '1';
        }
    }
    return bits;
}

void printAppliedOperation(const gate& operation) {
    std::cout << "Applied " << operation.name;
    if (!operation.targets.empty()) {
        std::cout << " ->";
        for (const int target : operation.targets) {
            std::cout << " q[" << target << "]";
        }
    } else if (operation.type == GateType::RegisterTransform) {
        std::cout << " -> reg[" << static_cast<std::size_t>(operation.parameters[0]) << "]";
    } else if (operation.type == GateType::CustomRegisterMatrix) {
        std::cout << " -> reg[" << operation.registerId << "]";
    }
    if (!operation.controls.empty()) {
        std::cout << " (controls:";
        for (const int control : operation.controls) {
            std::cout << " q[" << control << "]";
        }
        std::cout << ")";
    }
    std::cout << '\n';
}

} // namespace

simulator::simulator(const circuit& circuit, const operators& operations, bool printOutput)
    : m_circuit(&circuit), m_operators(&operations), m_printOutput(printOutput) {}

void simulator::validate(const circuit& circuit, const operators& operations) const {
    const std::size_t qubitCount = circuit.numQubits();
    const std::size_t registerCount = circuit.numRegisters();
    const auto& gates = operations.getGates();

    const auto mismatch = [](std::size_t gateIndex, const gate& currentGate, const std::string& detail) {
        std::ostringstream message;
        message << "Circuit-operation mismatch at operation " << gateIndex
                << " ('" << currentGate.name << "'): " << detail;
        throw circuit_operation_mismatch(message.str());
    };
    const auto requireWire = [&](int wire, std::size_t gateIndex, const gate& currentGate) {
        if (wire < 0 || static_cast<std::size_t>(wire) >= qubitCount) {
            mismatch(gateIndex, currentGate, "wire index is outside the bound circuit");
        }
    };
    const auto requireRegister = [&](double id, std::size_t gateIndex, const gate& currentGate) {
        if (!std::isfinite(id) || id < 0.0 || std::floor(id) != id ||
            id >= static_cast<double>(registerCount)) {
            mismatch(gateIndex, currentGate, "register ID is outside the bound circuit");
        }
    };
    const auto requireRegisterTargets = [&](int registerId, std::size_t gateIndex, const gate& currentGate) {
        if (registerId < 0 || static_cast<std::size_t>(registerId) >= registerCount) {
            mismatch(gateIndex, currentGate, "register ID is outside the bound circuit");
        }
        const auto reg = circuit(static_cast<std::size_t>(registerId));
        if (reg.size() == 0) {
            mismatch(gateIndex, currentGate, "register observation cannot target an empty register");
        }
        if (currentGate.targets.size() != reg.size()) {
            mismatch(gateIndex, currentGate, "register operation targets do not match the selected register");
        }
        for (std::size_t local = 0; local < reg.size(); ++local) {
            if (currentGate.targets[local] != static_cast<int>(reg.startIndex() + local)) {
                mismatch(gateIndex, currentGate, "register operation targets do not match the selected register");
            }
        }
    };

    // Validate once before execution so malformed operations cannot partially run.
    for (std::size_t gateIndex = 0; gateIndex < gates.size(); ++gateIndex) {
        const gate& operation = gates[gateIndex];
        if (operation.sourceCircuit != nullptr && operation.sourceCircuit != &circuit) {
            mismatch(gateIndex, operation, "operation was created from a different circuit timeline");
        }
        for (const int target : operation.targets) {
            requireWire(target, gateIndex, operation);
        }
        for (const int control : operation.controls) {
            requireWire(control, gateIndex, operation);
        }
        if (!hasDistinctWires(operation)) {
            mismatch(gateIndex, operation, "target and control wires must be distinct");
        }

        switch (operation.type) {
        case GateType::Standard:
            if (operation.name == "X" || operation.name == "Y" || operation.name == "Z" ||
                operation.name == "H" || operation.name == "T") {
                if (operation.targets.size() != 1 || !operation.controls.empty()) {
                    mismatch(gateIndex, operation, "expected one target and no controls");
                }
            } else if (operation.name == "CX" || operation.name == "CY" || operation.name == "CZ") {
                if (operation.targets.size() != 1 || operation.controls.size() != 1) {
                    mismatch(gateIndex, operation, "expected one target and one control");
                }
            } else if (operation.name == "SWAP") {
                if (operation.targets.size() != 2 || !operation.controls.empty()) {
                    mismatch(gateIndex, operation, "expected two targets and no controls");
                }
            } else if (operation.name == "CCX") {
                if (operation.targets.size() != 1 || operation.controls.size() != 2) {
                    mismatch(gateIndex, operation, "expected one target and two controls");
                }
            } else {
                mismatch(gateIndex, operation, "unrecognized standard operation");
            }
            break;

        case GateType::Parameterized:
            if (operation.targets.size() != 1 || !operation.controls.empty() ||
                operation.parameters.size() != 1 ||
                (operation.name != "Rx" && operation.name != "Ry" && operation.name != "Rz") ||
                !std::isfinite(operation.parameters[0])) {
                mismatch(gateIndex, operation, "invalid parameterized single-wire operation");
            }
            break;

        case GateType::CustomMatrix:
            if (operation.name != "CustomU" || operation.targets.size() != 1 ||
                !operation.controls.empty() || !operation.parameters.empty() ||
                !isUnitary2x2(operation.explicitMatrix)) {
                mismatch(gateIndex, operation, "expected one target and a finite 2x2 unitary matrix without parameters");
            }
            break;

        case GateType::CustomRegisterMatrix: {
            const bool controlled = operation.name == "ControlledCustomRegisterU";
            if ((!controlled && operation.name != "CustomRegisterU") ||
                !operation.targets.empty() ||
                (controlled ? operation.controls.size() != 1 : !operation.controls.empty()) ||
                !operation.parameters.empty()) {
                mismatch(gateIndex, operation, "invalid custom-register operation metadata");
            }
            if (operation.registerId < 0 ||
                static_cast<std::size_t>(operation.registerId) >= registerCount) {
                mismatch(gateIndex, operation, "custom matrix register ID is outside the bound circuit");
            }
            const auto reg = circuit(static_cast<std::size_t>(operation.registerId));
            const std::size_t registerWidth = reg.size();
            if (registerWidth == 0 ||
                registerWidth >= std::numeric_limits<std::size_t>::digits ||
                reg.startIndex() >= std::numeric_limits<std::size_t>::digits ||
                registerWidth > std::numeric_limits<std::size_t>::digits - reg.startIndex()) {
                mismatch(gateIndex, operation, "register is too large for matrix basis indexing");
            }
            if (controlled) {
                const std::size_t registerEnd = reg.startIndex() + registerWidth;
                const std::size_t control = static_cast<std::size_t>(operation.controls[0]);
                if (control >= reg.startIndex() && control < registerEnd) {
                    mismatch(gateIndex, operation, "controlled register matrix control must be outside its target register");
                }
            }
            const std::size_t dimension = std::size_t{1} << registerWidth;
            if (!isUnitary(operation.explicitMatrix, dimension)) {
                mismatch(
                    gateIndex,
                    operation,
                    "custom register matrix must be a finite unitary matrix with dimension 2^register-size");
            }
            break;
        }

        case GateType::Controlled:
        case GateType::MultiControlled:
            if ((operation.type == GateType::Controlled && operation.name != "CU") ||
                (operation.type == GateType::MultiControlled && operation.name != "MCU") ||
                operation.targets.size() != 1 || !operation.parameters.empty() ||
                (operation.type == GateType::Controlled && operation.controls.size() != 1) ||
                (operation.type == GateType::MultiControlled && operation.controls.empty())) {
                mismatch(gateIndex, operation, "invalid controlled-operation target or controls");
            }
            if (!isUnitary2x2(operation.explicitMatrix)) {
                mismatch(gateIndex, operation, "controlled matrix must be a finite 2x2 unitary without parameters");
            }
            break;

        case GateType::Measurement:
            if (operation.name == "MEASURE") {
                if (operation.targets.size() != 1 || !operation.controls.empty() ||
                    operation.classicalBit < 0 || operation.registerId != -1) {
                    mismatch(gateIndex, operation, "invalid measurement target or classical bit");
                }
            } else if (operation.name == "MEASURE_REGISTER") {
                if (!operation.controls.empty() || operation.classicalBit != -1) {
                    mismatch(gateIndex, operation, "invalid register measurement metadata");
                }
                requireRegisterTargets(operation.registerId, gateIndex, operation);
            } else {
                mismatch(gateIndex, operation, "unrecognized measurement operation");
            }
            break;

        case GateType::Sampling:
            if ((operation.name != "SAMPLER" && operation.name != "SAMPLER_REGISTER") ||
                !operation.controls.empty() || operation.parameters.size() != 1 ||
                !std::isfinite(operation.parameters[0]) || operation.parameters[0] < 1.0 ||
                std::floor(operation.parameters[0]) != operation.parameters[0] ||
                operation.parameters[0] > static_cast<double>(std::numeric_limits<int>::max())) {
                mismatch(gateIndex, operation, "sampler requires a positive integer shot count");
            }
            if (operation.name == "SAMPLER") {
                if (operation.targets.size() != 1 || operation.registerId != -1) {
                    mismatch(gateIndex, operation, "single-wire sampler requires one target");
                }
            } else {
                requireRegisterTargets(operation.registerId, gateIndex, operation);
                if (operation.targets.size() >= std::numeric_limits<std::size_t>::digits) {
                    mismatch(gateIndex, operation, "sampled register is too large for basis indexing");
                }
            }
            break;

        case GateType::Barrier:
            if (!operation.controls.empty()) {
                mismatch(gateIndex, operation, "barriers cannot have control wires");
            }
            break;

        case GateType::RegisterTransform:
            if (!operation.targets.empty() || !operation.controls.empty() ||
                operation.parameters.empty()) {
                mismatch(gateIndex, operation, "expected register ID metadata and no wire operands");
            }
            requireRegister(operation.parameters[0], gateIndex, operation);
            if (operation.registerId >= 0 &&
                static_cast<double>(operation.registerId) != operation.parameters[0]) {
                mismatch(gateIndex, operation, "register-transform metadata does not match its register ID");
            }
            if (operation.name == "GROVER_DIFFUSION") {
                if (operation.parameters.size() != 2 ||
                    !std::isfinite(operation.parameters[1]) ||
                    operation.parameters[1] < 0.0 ||
                    std::floor(operation.parameters[1]) != operation.parameters[1]) {
                    mismatch(gateIndex, operation, "diffusion iterations must be a nonnegative integer");
                }
            } else if ((operation.name == "QFT" || operation.name == "IQFT" ||
                        operation.name == "WALSH_HADAMARD") &&
                       operation.parameters.size() != 1) {
                mismatch(gateIndex, operation, "unexpected register-transform metadata");
            } else if (operation.name != "QFT" && operation.name != "IQFT" &&
                       operation.name != "WALSH_HADAMARD" &&
                       operation.name != "GROVER_DIFFUSION") {
                mismatch(gateIndex, operation, "unrecognized register transform");
            }
            break;
        }
    }
}

void simulator::initializeState() {
    const std::size_t qubitCount = m_circuit->numQubits();
    if (qubitCount >= std::numeric_limits<std::size_t>::digits) {
        throw std::length_error("Circuit is too large for state-vector indexing");
    }

    const std::size_t stateCount = std::size_t{1} << qubitCount;
    if (stateCount > m_stateVector.max_size()) {
        throw std::length_error("Circuit state vector exceeds the maximum supported size");
    }
    m_stateVector.assign(stateCount, Complex{0.0, 0.0});
    m_stateVector[0] = Complex{1.0, 0.0};
}

void simulator::run() {
    m_duration = std::chrono::duration<double, std::milli>::zero();
    m_stateVector.clear();
    validate(*m_circuit, *m_operators);
    initializeState();

    std::mt19937_64 generator(std::random_device{}());
    // Gates update the shared state in timeline order; observation gates use snapshots.
    for (const gate& operation : m_operators->getGates()) {
        const auto start = std::chrono::high_resolution_clock::now();
        switch (operation.type) {
        case GateType::Standard:
            applyStandardGate(m_stateVector, operation);
            break;

        case GateType::Parameterized:
            applyNamedGate(m_stateVector, operation, operation.name, {});
            break;

        case GateType::CustomMatrix:
            applyExplicitMatrix(m_stateVector, operation.targets[0], {}, operation.explicitMatrix);
            break;

        case GateType::CustomRegisterMatrix: {
            const auto reg = (*m_circuit)(static_cast<std::size_t>(operation.registerId));
            applyRegisterMatrix(
                m_stateVector,
                reg.startIndex(),
                reg.size(),
                operation.explicitMatrix,
                operation.controls);
            break;
        }

        case GateType::Controlled:
        case GateType::MultiControlled:
            applyControlledGate(m_stateVector, operation);
            break;

        case GateType::Measurement: {
            if (operation.name == "MEASURE_REGISTER") {
                const auto probabilities = registerProbabilities(m_stateVector, operation.targets);
                const std::size_t outcome =
                    std::discrete_distribution<std::size_t>(probabilities.begin(), probabilities.end())(generator);
                if (m_printOutput) {
                    std::cout << "measure() -> reg[" << operation.registerId << "] = 0b"
                              << registerBitString(outcome, operation.targets.size())
                              << " (decimal " << outcome << ")\n";
                }
            } else {
                const double probabilityOne = oneProbability(m_stateVector, operation.targets[0]);
                const int result = std::bernoulli_distribution(probabilityOne)(generator) ? 1 : 0;
                if (m_printOutput) {
                    std::cout << "measure(). measured q[" << operation.targets[0] << "] = "
                              << result << '\n';
                }
            }
            break;
        }

        case GateType::Sampling: {
            const int shots = static_cast<int>(operation.parameters[0]);
            if (operation.name == "SAMPLER_REGISTER") {
                const auto probabilities = registerProbabilities(m_stateVector, operation.targets);
                std::discrete_distribution<std::size_t> distribution(
                    probabilities.begin(), probabilities.end());
                std::vector<int> counts(probabilities.size(), 0);
                for (int shot = 0; shot < shots; ++shot) {
                    ++counts[distribution(generator)];
                }
                if (m_printOutput) {
                    std::cout << "sampler() -> sampled reg[" << operation.registerId
                              << "] | shots = " << shots << '\n';
                    for (std::size_t outcome = 0; outcome < counts.size(); ++outcome) {
                        if (counts[outcome] != 0) {
                            std::cout << "  0b" << registerBitString(outcome, operation.targets.size())
                                      << " (decimal " << outcome << "): " << counts[outcome] << '\n';
                        }
                    }
                }
            } else {
                const double probabilityOne = oneProbability(m_stateVector, operation.targets[0]);
                const int ones = std::binomial_distribution<int>(shots, probabilityOne)(generator);
                const int zeros = shots - ones;
                if (m_printOutput) {
                    std::cout << "sampler() -> sampled q[" << operation.targets[0] << "] | shots = "
                              << shots << " || |0> " << zeros << " | |1> " << ones << '\n';
                }
            }
            break;
        }

        case GateType::Barrier:
            break;

        case GateType::RegisterTransform: {
            const auto reg = (*m_circuit)(static_cast<std::size_t>(operation.parameters[0]));
            if (operation.name == "QFT") {
                applyRegisterFourier(m_stateVector, reg.startIndex(), reg.size(), false);
            } else if (operation.name == "IQFT") {
                applyRegisterFourier(m_stateVector, reg.startIndex(), reg.size(), true);
            } else if (operation.name == "WALSH_HADAMARD") {
                applyWalshHadamard(m_stateVector, reg.startIndex(), reg.size());
            } else if (operation.name == "GROVER_DIFFUSION") {
                applyDiffusion(
                    m_stateVector,
                    reg.startIndex(),
                    reg.size(),
                    static_cast<std::size_t>(operation.parameters[1]));
            }
            break;
        }
        }
        const auto finish = std::chrono::high_resolution_clock::now();
        m_duration += finish - start;
        if (m_printOutput &&
            operation.type != GateType::Measurement &&
            operation.type != GateType::Sampling) {
            printAppliedOperation(operation);
        }
    }
}

const std::vector<Complex>& simulator::getStateVector() const noexcept {
    return m_stateVector;
}

void simulator::printStateVector() const {
    for (std::size_t basisState = 0; basisState < m_stateVector.size(); ++basisState) {
        std::cout << "|" << basisState << "> : " << m_stateVector[basisState] << '\n';
    }
}

std::chrono::duration<double, std::milli> simulator::duration() const noexcept {
    return m_duration;
}

} // namespace quantum
