cmake_minimum_required(VERSION 3.25)
project(QuantumCircuitStudio VERSION 0.4 LANGUAGES CXX)

# Require C++17 for std::complex and modern standard library features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force MSVC to correctly report C++17 compatibility to Qt
if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

# User Variables
set(application "QCS")
set(engine "QCE")

set(QT6_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.tools/Qt")

# =====================================================================
# Portable Qt6 Setup (Local Pre-compiled Binaries)
# =====================================================================

# 1. Define local Qt6Core
add_library(LocalQt6::Core SHARED IMPORTED)
set_target_properties(LocalQt6::Core PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${QT6_LOCAL_DIR}/include"
    IMPORTED_IMPLIB_DEBUG        "${QT6_LOCAL_DIR}/lib/Qt6Cored.lib"
    IMPORTED_LOCATION_DEBUG     "${QT6_LOCAL_DIR}/bin/Qt6Cored.dll"
    IMPORTED_IMPLIB_RELEASE      "${QT6_LOCAL_DIR}/lib/Qt6Core.lib"
    IMPORTED_LOCATION_RELEASE    "${QT6_LOCAL_DIR}/bin/Qt6Core.dll"
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
)

# 2. Define local Qt6Gui
add_library(LocalQt6::Gui SHARED IMPORTED)
set_target_properties(LocalQt6::Gui PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${QT6_LOCAL_DIR}/include"
    IMPORTED_IMPLIB_DEBUG        "${QT6_LOCAL_DIR}/lib/Qt6Guid.lib"
    IMPORTED_LOCATION_DEBUG     "${QT6_LOCAL_DIR}/bin/Qt6Guid.dll"
    IMPORTED_IMPLIB_RELEASE      "${QT6_LOCAL_DIR}/lib/Qt6Gui.lib"
    IMPORTED_LOCATION_RELEASE    "${QT6_LOCAL_DIR}/bin/Qt6Gui.dll"
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
)

# 3. Define local Qt6Widgets
add_library(LocalQt6::Widgets SHARED IMPORTED)
set_target_properties(LocalQt6::Widgets PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${QT6_LOCAL_DIR}/include"
    IMPORTED_IMPLIB_DEBUG        "${QT6_LOCAL_DIR}/lib/Qt6Widgetsd.lib"
    IMPORTED_LOCATION_DEBUG     "${QT6_LOCAL_DIR}/bin/Qt6Widgetsd.dll"
    IMPORTED_IMPLIB_RELEASE      "${QT6_LOCAL_DIR}/lib/Qt6Widgets.lib"
    IMPORTED_LOCATION_RELEASE    "${QT6_LOCAL_DIR}/bin/Qt6Widgets.dll"
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
)

# =====================================================================
# Reusable engine library
# =====================================================================

set(ENGINE_SOURCES
    engine/src/core/circuit.cpp
    engine/src/core/operators.cpp
    engine/src/core/simulator.cpp
    engine/src/registers/q_register.cpp
    engine/src/registers/qubit.cpp
    engine/src/costfxn.cpp
    engine/src/variational/ansatz.cpp
    engine/src/variational/optimizer.cpp
    engine/src/qml/qnn.cpp
    engine/src/qml/losses.cpp
    engine/src/qml/gradients.cpp
)

# Defines the "Quantum Circuit Engine" library target
add_library(${engine} STATIC ${ENGINE_SOURCES})

# Set Include Directories for the Engine
target_include_directories(${engine} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/include
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/include/qml
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/include/registers
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/include/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/include/variational
)

# =====================================================================
# Application Executable
# =====================================================================

# Defines the "Quantum Circuit Studio" executable target
add_executable(${application}
    main.cpp
    app/src/circuit_canvas.cpp
    app/include/circuit_canvas.hpp
)

# Allow code to find the canvas and local Qt headers
target_include_directories(${application} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/app/include
    ${QT6_LOCAL_DIR}/include
    ${QT6_LOCAL_DIR}/include/QtCore
    ${QT6_LOCAL_DIR}/include/QtGui
    ${QT6_LOCAL_DIR}/include/QtWidgets
    )

# Links the Engine and Portable Qt Libraries with the Application
target_link_libraries(${application} PRIVATE 
    ${engine}
    LocalQt6::Widgets
    LocalQt6::Gui
    LocalQt6::Core
)


# =====================================================================
# Automation: Copy Runtime Dependencies to Output Build Directory
# =====================================================================
add_custom_command(TARGET ${application} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT6_LOCAL_DIR}/bin/$<IF:$<CONFIG:Debug>,Qt6Cored.dll,Qt6Core.dll>"
        "${QT6_LOCAL_DIR}/bin/$<IF:$<CONFIG:Debug>,Qt6Guid.dll,Qt6Gui.dll>"
        "${QT6_LOCAL_DIR}/bin/$<IF:$<CONFIG:Debug>,Qt6Widgetsd.dll,Qt6Widgets.dll>"
        $<TARGET_FILE_DIR:${application}>
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${application}>/platforms
    # FIX: Uses qwindowsd.dll for Debug configuration, and qwindows.dll for Release configuration
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT6_LOCAL_DIR}/bin/plugins/platforms/$<IF:$<CONFIG:Debug>,qwindowsd.dll,qwindows.dll>"
        $<TARGET_FILE_DIR:${application}>/platforms/qwindows.dll
    COMMENT "Deploying local Qt runtime libraries and environment plugins..."
)
