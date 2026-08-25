cmake_minimum_required(VERSION 3.20)
project(QuantumSimulationStudio VERSION 0.3.5 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Require C++17 for std::complex and modern standard library features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================
# Reusable engine library
# =============================

set(ENGINE_SOURCES
    src/core/QuantumCircuit.cpp
    src/core/QuantumSimulator.cpp
    src/registers/QuantumRegister.cpp
    src/core/Qubit.cpp
    src/algorithms/AlgorithmLibrary.cpp
    src/algorithms/GroverShor.cpp
    src/variational/Variational.cpp
    src/qnn/QNN.cpp
    src/qnn/Gradients.cpp
    src/qnn/Losses.cpp
)

add_library(quantum_engine STATIC ${ENGINE_SOURCES})
add_library(QCS::QCS ALIAS quantum_engine)
set_target_properties(quantum_engine PROPERTIES EXPORT_NAME QCS)
# Keep the language requirement visible to downstream applications.
target_compile_features(quantum_engine PUBLIC cxx_std_17)
target_include_directories(quantum_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/core>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/registers>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/algorithms>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/variational>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/qnn>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/core>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/registers>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/algorithms>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/variational>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qnn>
)

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

install(TARGETS quantum_engine
    EXPORT QCSTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT QCSTargets
    FILE QCSTargets.cmake
    NAMESPACE QCS::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QCS
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/QCSConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/QCSConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QCS
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/QCSConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/QCSConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/QCSConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QCS
)