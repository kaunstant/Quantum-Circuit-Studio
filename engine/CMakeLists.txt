cmake_minimum_required(VERSION 3.20)
project(QuantumSimulationStudio VERSION 0.2 LANGUAGES CXX)

# Require C++17 for std::complex and modern standard library features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ENGINE_SOURCES
    src/core/QuantumCircuit.cpp
    src/core/QuantumSimulator.cpp
    src/registers/QuantumRegister.cpp
    src/core/Qubit.cpp
    src/algorithms/AlgorithmLibrary.cpp
    src/algorithms/GroverShor.cpp
    src/variational/Variational.cpp
)

add_library(quantum_engine STATIC ${ENGINE_SOURCES})
target_include_directories(quantum_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/core ${CMAKE_CURRENT_SOURCE_DIR}/include/registers ${CMAKE_CURRENT_SOURCE_DIR}/include/algorithms ${CMAKE_CURRENT_SOURCE_DIR}/include/variational)

add_executable(QuantumSimulationStudio main.cpp)
target_include_directories(QuantumSimulationStudio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/console
    ${CMAKE_CURRENT_SOURCE_DIR}/../examples
)
target_link_libraries(QuantumSimulationStudio PRIVATE quantum_engine)

file(GLOB EXAMPLE_PROGRAMS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../examples/*.cpp)
foreach(EXAMPLE_SOURCE IN LISTS EXAMPLE_PROGRAMS)
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})
    target_include_directories(${EXAMPLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/console)
    target_include_directories(${EXAMPLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../examples)
    target_link_libraries(${EXAMPLE_NAME} PRIVATE quantum_engine)
endforeach()