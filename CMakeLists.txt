# =============================================================================
#  CMakeLists.txt - Simulador de Algoritmos de Planificación de CPU (Equipo 6)
#
#  VS Code + CMake Tools (Windows y macOS): abrir esta carpeta, elegir el kit
#  (MSVC Build Tools x64 en Windows, Apple Clang en macOS) y compilar con F7.
#
#  Cada integrante agrega sus .cpp a la biblioteca planificador_core
#  (por ejemplo Core/algorithms/round_robin.cpp o Core/metrics.cpp).
# =============================================================================
cmake_minimum_required(VERSION 3.20)
project(SimuladorPlanificacion LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # /utf-8: los archivos están en UTF-8 (tildes en mensajes y comentarios).
    # /permissive-: cumplimiento estricto del estándar.
    add_compile_options(/W4 /permissive- /utf-8 /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Motor y algoritmos ---------------------------------------------------
add_library(planificador_core STATIC
    Core/process.cpp
    Core/scheduler.cpp
    Core/algorithms/fcfs.cpp
    Core/algorithms/sjf.cpp
    # Core/algorithms/round_robin.cpp   (Estratega de Turnos)
    # Core/algorithms/priority.cpp      (Estratega de Turnos)
    # Core/metrics.cpp                  (Analista de Rendimiento)
    # Utils/file_handler.cpp            (Guardián de Pruebas y Archivos)
    # Utils/process_generator.cpp       (Guardián de Pruebas y Archivos)
)
target_include_directories(planificador_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# --- Pruebas automáticas del motor ---------------------------------------
add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE planificador_core)

enable_testing()
add_test(NAME pruebas_motor COMMAND test_core)

# --- Demostración del hito H1 --------------------------------------------
add_executable(demo_motor demo/demo_motor.cpp)
target_link_libraries(demo_motor PRIVATE planificador_core)
