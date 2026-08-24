cmake_minimum_required(VERSION 3.20)
project(QuantumSimulationStudio VERSION 0.1 LANGUAGES CXX )

# Require C++17 for std::complex and modern standard library features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the headers directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Define source files
set(SOURCES
    main.cpp
    core/QuantumCircuit.cpp
    core/QuantumSimulator.cpp
)

# Add executable
add_executable(QuantumSimulationStudio ${SOURCES})