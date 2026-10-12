import argparse
import csv
import math
import time
from fractions import Fraction
from pathlib import Path

import numpy as np
from qiskit import QuantumCircuit, transpile
from qiskit.circuit.library import QFTGate, UnitaryGate
from qiskit.quantum_info import SparsePauliOp, Statevector
from qiskit_aer import AerSimulator


ROOT = Path(__file__).resolve().parents[2]
DATA_PATH = ROOT / "example" / "Vine Quality Predictor" / "dataset" / "data.csv"
BACKEND = AerSimulator(method="statevector")
PI = math.pi
H2_TERMS = [
    (-1.052373245772859, "II"),
    (0.39793742484318045, "ZI"),
    (-0.39793742484318045, "IZ"),
    (-0.01128010425623538, "ZZ"),
    (0.18093119978423156, "XX"),
]
H2_OPERATOR = SparsePauliOp.from_list(
    [(pauli[::-1], coefficient) for coefficient, pauli in H2_TERMS]
)


def run_shots(circuit, shots, seed=1234):
    compiled = transpile(circuit, BACKEND, optimization_level=0)
    return BACKEND.run(
        compiled,
        shots=shots,
        seed_simulator=seed,
    ).result().get_counts()


def statevector(circuit):
    return Statevector.from_instruction(circuit)


def make_grover_circuit():
    circuit = QuantumCircuit(3, 3)
    circuit.h(range(3))
    for _ in range(2):
        circuit.x(1)
        circuit.h(2)
        circuit.ccx(0, 1, 2)
        circuit.h(2)
        circuit.x(1)

        circuit.h(range(3))
        circuit.x(range(3))
        circuit.h(2)
        circuit.ccx(0, 1, 2)
        circuit.h(2)
        circuit.x(range(3))
        circuit.h(range(3))
    circuit.measure(range(3), range(3))
    return circuit


def run_bell():
    circuit = QuantumCircuit(2, 2)
    circuit.h(0)
    circuit.cx(0, 1)
    circuit.measure(range(2), range(2))
    counts = run_shots(circuit, 1024)
    return {"shots": 1024, "counts": counts}


def run_grover():
    counts = run_shots(make_grover_circuit(), 1024)
    return {"wires": 3, "iterations": 2, "shots": 1024, "counts": counts}


def run_qft():
    circuit = QuantumCircuit(3)
    circuit.x(0)
    circuit.x(1)
    circuit.append(QFTGate(3), range(3))
    result = statevector(circuit)
    return {"wires": 3, "state_norm": float(np.linalg.norm(result.data))}


def run_phase_estimation():
    phase_wires = 3
    target = phase_wires
    circuit = QuantumCircuit(phase_wires + 1, phase_wires)
    circuit.x(target)
    circuit.h(target)
    for bit in range(phase_wires):
        control = bit
        circuit.h(control)
        if (1 << bit) & 1:
            circuit.cx(control, target)
    circuit.compose(
        QFTGate(phase_wires).inverse().definition,
        qubits=range(phase_wires),
        inplace=True,
    )
    circuit.measure(range(phase_wires), range(phase_wires))
    return {
        "phase_wires": phase_wires,
        "shots": 256,
        "counts": run_shots(circuit, 256),
    }


def modular_multiplication_matrix(multiplier, modulus, width):
    dimension = 1 << width
    matrix = np.zeros((dimension, dimension), dtype=complex)
    for value in range(dimension):
        mapped = (multiplier * value) % modulus if value < modulus else value
        matrix[mapped, value] = 1.0
    return matrix


def recover_order(counts, phase_bits, base=2, modulus=15):
    denominator = 1 << phase_bits
    for bitstring, count in sorted(counts.items(), key=lambda item: item[1], reverse=True):
        phase = int(bitstring.replace(" ", ""), 2)
        if phase == 0:
            continue
        candidate = Fraction(phase, denominator).limit_denominator(modulus)
        order = candidate.denominator
        if order > 1 and order < modulus and pow(base, order, modulus) == 1:
            return order
    return 0


def run_shor():
    number, base = 15, 2
    phase_bits = 4
    target_bits = 4
    target_wires = list(range(phase_bits, phase_bits + target_bits))
    circuit = QuantumCircuit(phase_bits + target_bits, phase_bits)
    circuit.x(target_wires[0])

    for bit in range(phase_bits):
        control = bit
        circuit.h(control)
        power = 1 << bit
        multiplier = pow(base, power, number)
        matrix = modular_multiplication_matrix(multiplier, number, target_bits)
        controlled = UnitaryGate(matrix, label=f"multiply_{multiplier}").control()
        circuit.append(controlled, [control] + target_wires)

    circuit.compose(
        QFTGate(phase_bits).inverse().definition,
        qubits=range(phase_bits),
        inplace=True,
    )
    circuit.measure(range(phase_bits), range(phase_bits))
    counts = run_shots(circuit, 1024)
    order = recover_order(counts, phase_bits, base, number)
    factors = ()
    if order and order % 2 == 0:
        half_power = pow(base, order // 2, number)
        factors = (math.gcd(half_power - 1, number), math.gcd(half_power + 1, number))
    return {
        "N": number,
        "a": base,
        "phase_wires": phase_bits,
        "shots": 1024,
        "order": order,
        "factors": factors,
    }


def qaoa_state(parameters):
    circuit = QuantumCircuit(2)
    circuit.h(0)
    circuit.h(1)
    circuit.cx(0, 1)
    circuit.rz(parameters[0], 1)
    circuit.cx(0, 1)
    circuit.rx(2.0 * parameters[1], 0)
    circuit.rx(2.0 * parameters[1], 1)
    return statevector(circuit)


def qaoa_cost(parameters):
    probabilities = np.abs(qaoa_state(parameters).data) ** 2
    cut_probability = sum(
        probability
        for basis, probability in enumerate(probabilities)
        if (basis & 1) != ((basis >> 1) & 1)
    )
    return -float(cut_probability)


def adam_minimize(objective, initial_parameters, max_iterations=300, tolerance=1e-7):
    parameters = np.asarray(initial_parameters, dtype=float)
    first_moment = np.zeros_like(parameters)
    second_moment = np.zeros_like(parameters)
    energy = objective(parameters)
    iterations = 0
    beta_one, beta_two = 0.9, 0.999

    for iteration in range(1, max_iterations + 1):
        gradient = np.empty_like(parameters)
        for index in range(parameters.size):
            plus = parameters.copy()
            minus = parameters.copy()
            plus[index] += 1e-5
            minus[index] -= 1e-5
            gradient[index] = (objective(plus) - objective(minus)) / 2e-5
        first_moment = beta_one * first_moment + (1 - beta_one) * gradient
        second_moment = beta_two * second_moment + (1 - beta_two) * gradient**2
        corrected_first = first_moment / (1 - beta_one**iteration)
        corrected_second = second_moment / (1 - beta_two**iteration)
        next_parameters = parameters - 0.05 * corrected_first / (
            np.sqrt(corrected_second) + 1e-8
        )
        next_energy = objective(next_parameters)
        iterations = iteration
        change = abs(next_energy - energy)
        parameters, energy = next_parameters, next_energy
        if change <= tolerance and np.linalg.norm(gradient) <= math.sqrt(tolerance):
            break
    return {"x": parameters, "fun": energy, "nit": iterations}


def run_vqe():
    def objective(parameters):
        circuit = QuantumCircuit(2)
        parameter = 0
        for _ in range(2):
            for wire in range(2):
                circuit.ry(parameters[parameter], wire)
                parameter += 1
                circuit.rz(parameters[parameter], wire)
                parameter += 1
            circuit.cx(0, 1)
        return float(statevector(circuit).expectation_value(H2_OPERATOR).real)

    result = adam_minimize(
        objective, [0.2 + 0.17 * index for index in range(8)]
    )
    return {"Hamiltonian": H2_TERMS, "iterations": result["nit"], "energy": result["fun"]}


def run_qaoa():
    result = adam_minimize(qaoa_cost, [0.2, 0.37])
    return {
        "graph": "2 vertices, 1 edge",
        "depth": 1,
        "iterations": result["nit"],
        "expected_cut": -result["fun"],
    }


def read_wine_rows():
    with DATA_PATH.open(newline="", encoding="utf-8") as data_file:
        rows = list(csv.reader(data_file))
    return [[float(value) for value in row] for row in rows[1:] if row]


def wine_subset():
    rows = read_wine_rows()
    indices = [round(index * (len(rows) - 1) / 11) for index in range(12)]
    return [rows[index] for index in indices]


def scale_wine_features(training_rows, all_rows):
    features = np.asarray([row[:-1] for row in training_rows], dtype=float)
    minimum = features.min(axis=0)
    maximum = features.max(axis=0)

    def transform(rows):
        output = []
        for row in rows:
            values = []
            for index, raw in enumerate(row[:-1]):
                width = maximum[index] - minimum[index]
                values.append(0.0 if width == 0.0 else (raw - minimum[index]) / width * PI)
            output.append(values)
        return np.asarray(output, dtype=float)

    return transform(all_rows)


def wine_classification_circuit(features, weights):
    circuit = QuantumCircuit(3)
    for index, value in enumerate(features):
        wire = index % 3
        if (index // 3) % 3 == 0:
            circuit.rx(value, wire)
        elif (index // 3) % 3 == 1:
            circuit.ry(value, wire)
        else:
            circuit.rz(value, wire)
    for wire in range(3):
        circuit.ry(weights[wire * 2], wire)
        circuit.rz(weights[wire * 2 + 1], wire)
    circuit.cx(0, 1)
    circuit.cx(1, 2)
    return circuit


def one_probability(circuit, qubit=0):
    return float(statevector(circuit).probabilities([qubit])[1])


def wine_classification_output(features, weights):
    return one_probability(wine_classification_circuit(features, weights))


def qnn_regression_output(value, weights):
    circuit = QuantumCircuit(1)
    circuit.ry(value, 0)
    circuit.ry(float(weights[0]), 0)
    return float(statevector(circuit).expectation_value(SparsePauliOp("Z")).real)


def train_scalar_qnn(inputs, targets, output, classification, epochs, batch_size, rate, initial):
    weights = np.asarray(initial, dtype=float)
    first_moment = np.zeros_like(weights)
    second_moment = np.zeros_like(weights)
    update = 0
    beta_one, beta_two = 0.9, 0.999

    for _ in range(epochs):
        for start in range(0, len(inputs), batch_size):
            stop = min(start + batch_size, len(inputs))
            gradient = np.zeros_like(weights)
            for sample in range(start, stop):
                for parameter in range(weights.size):
                    plus = weights.copy()
                    minus = weights.copy()
                    plus[parameter] += PI / 2
                    minus[parameter] -= PI / 2
                    if classification:
                        positive = output(inputs[sample], plus)
                        negative = output(inputs[sample], minus)
                        derivative = (positive - negative) / 2.0
                        probability = output(inputs[sample], weights)
                        target = targets[sample]
                        if probability <= 1e-12 or probability >= 1 - 1e-12:
                            derivative = 0.0
                        else:
                            derivative *= (probability - target) / (
                                probability * (1 - probability)
                            )
                    else:
                        positive = output(inputs[sample], plus)
                        negative = output(inputs[sample], minus)
                        prediction = output(inputs[sample], weights)
                        derivative = (positive - negative) / 2.0
                        derivative *= 2.0 * (prediction - targets[sample])
                    gradient[parameter] += derivative
            gradient /= stop - start
            update += 1
            first_moment = beta_one * first_moment + (1 - beta_one) * gradient
            second_moment = beta_two * second_moment + (1 - beta_two) * gradient**2
            corrected_first = first_moment / (1 - beta_one**update)
            corrected_second = second_moment / (1 - beta_two**update)
            weights -= rate * corrected_first / (np.sqrt(corrected_second) + 1e-8)
    return weights


def run_qnn_classification():
    rows = wine_subset()
    training_rows, test_rows = rows[:8], rows[8:]
    training_features = scale_wine_features(training_rows, training_rows)
    test_features = scale_wine_features(training_rows, test_rows)
    training_targets = [float(row[-1] >= 6) for row in training_rows]
    test_targets = np.asarray([float(row[-1] >= 6) for row in test_rows])
    weights = train_scalar_qnn(
        training_features,
        training_targets,
        wine_classification_output,
        True,
        epochs=1,
        batch_size=4,
        rate=0.02,
        initial=[0.05] * 6,
    )
    predictions = np.asarray(
        [wine_classification_output(features, weights) for features in test_features]
    )
    accuracy = float(np.mean((predictions >= 0.5) == test_targets))
    return {
        "training_rows": len(training_rows),
        "test_rows": len(test_rows),
        "epochs": 1,
        "batch_size": 4,
        "accuracy": accuracy,
    }


def run_qnn_regression():
    inputs = [0.0, 0.7, 1.4]
    targets = [math.cos(value) for value in inputs]
    weights = train_scalar_qnn(
        inputs,
        targets,
        qnn_regression_output,
        False,
        epochs=60,
        batch_size=3,
        rate=0.15,
        initial=[0.6],
    )
    predictions = np.asarray([qnn_regression_output(value, weights) for value in inputs])
    rmse = float(np.sqrt(np.mean((predictions - targets) ** 2)))
    return {"training_rows": len(inputs), "epochs": 60, "rmse": rmse}


WORKLOADS = {
    "bell": run_bell,
    "grover": run_grover,
    "qft": run_qft,
    "phase-estimation": run_phase_estimation,
    "shor": run_shor,
    "vqe": run_vqe,
    "qaoa": run_qaoa,
    "qnn-classification": run_qnn_classification,
    "qnn-regression": run_qnn_regression,
}


def main():
    parser = argparse.ArgumentParser(
        description="Run one Qiskit workload and print a single timed sample."
    )
    parser.add_argument("workload", choices=WORKLOADS)
    args = parser.parse_args()

    start = time.perf_counter()
    result = WORKLOADS[args.workload]()
    elapsed_ms = (time.perf_counter() - start) * 1000.0
    print(f"workload={args.workload}")
    print(f"end_to_end_wall_time_ms={elapsed_ms:.6f}")
    print(f"timed_boundary=end-to-end workload, excluding imports and console output")
    for key, value in result.items():
        print(f"{key}={value}")


if __name__ == "__main__":
    main()
