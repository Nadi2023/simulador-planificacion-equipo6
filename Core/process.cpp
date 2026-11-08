// =============================================================================
//  Archivo     : Core/process.cpp
//  Proyecto    : Simulador de Algoritmos de Planificación de CPU
//  Responsable : Nadibeth Buiz (Arquitecta de Planificación) - Equipo 6
//
//  Descripción :
//    Implementación de la clase Process: validación de datos de entrada y
//    transiciones de estado del modelo Nuevo / Listo / Ejecutando / Terminado.
// =============================================================================
#include "Core/process.h"

#include <cctype>
#include <stdexcept>
#include <utility>

// -----------------------------------------------------------------------------
//  Funciones del enum ProcessState
// -----------------------------------------------------------------------------
std::string toString(ProcessState state) {
    switch (state) {
        case ProcessState::New:        return "Nuevo";
        case ProcessState::Ready:      return "Listo";
        case ProcessState::Running:    return "Ejecutando";
        case ProcessState::Terminated: return "Terminado";
    }
    return "Desconocido";  // no se alcanza; evita advertencias del compilador
}

bool isValidTransition(ProcessState from, ProcessState to) noexcept {
    switch (from) {
        case ProcessState::New:
            return to == ProcessState::Ready;
        case ProcessState::Ready:
            return to == ProcessState::Running;
        case ProcessState::Running:
            return to == ProcessState::Ready || to == ProcessState::Terminated;
        case ProcessState::Terminated:
            return false;  // un proceso terminado no vuelve a ningún estado
    }
    return false;
}

// -----------------------------------------------------------------------------
//  Validación y construcción
// -----------------------------------------------------------------------------
std::string Process::validate(const std::string& id, int arrivalTime,
                              int burstTime, int priority) {
    if (id.empty()) {
        return "El ID del proceso no puede estar vacío.";
    }
    for (char c : id) {
        if (std::isspace(static_cast<unsigned char>(c)) != 0 || c == ',' || c == ';') {
            return "El ID \"" + id + "\" no puede contener espacios, comas ni punto y coma.";
        }
    }
    if (arrivalTime < 0) {
        return "El proceso " + id + " tiene un tiempo de llegada negativo (" +
               std::to_string(arrivalTime) + "); debe ser 0 o mayor.";
    }
    if (burstTime <= 0) {
        return "El proceso " + id + " tiene una ráfaga de " + std::to_string(burstTime) +
               "; la ráfaga debe ser mayor que 0.";
    }
    if (priority < 0) {
        return "El proceso " + id + " tiene una prioridad negativa (" +
               std::to_string(priority) + "); debe ser 0 o mayor.";
    }
    return "";
}

Process::Process(std::string id, int arrivalTime, int burstTime, int priority)
    : id_(std::move(id)),
      arrivalTime_(arrivalTime),
      burstTime_(burstTime),
      priority_(priority),
      remainingTime_(burstTime) {
    const std::string error = validate(id_, arrivalTime_, burstTime_, priority_);
    if (!error.empty()) {
        throw std::invalid_argument(error);
    }
}

// -----------------------------------------------------------------------------
//  Transiciones de estado (solo las usa el Scheduler)
// -----------------------------------------------------------------------------
void Process::changeState(ProcessState next) {
    if (!isValidTransition(state_, next)) {
        throw std::logic_error("Transición inválida en " + id_ + ": " +
                               toString(state_) + " -> " + toString(next) + ".");
    }
    state_ = next;
}

void Process::admit() {
    changeState(ProcessState::Ready);
}

void Process::dispatch(int now) {
    changeState(ProcessState::Running);
    if (startTime_ < 0) {
        startTime_ = now;  // primera vez que usa la CPU
    }
}

void Process::execute(int units) {
    if (state_ != ProcessState::Running) {
        throw std::logic_error("El proceso " + id_ +
                               " intentó ejecutarse sin estar en estado Ejecutando.");
    }
    if (units <= 0 || units > remainingTime_) {
        throw std::logic_error("El proceso " + id_ + " no puede ejecutar " +
                               std::to_string(units) + " unidades: le quedan " +
                               std::to_string(remainingTime_) + ".");
    }
    remainingTime_ -= units;
}

void Process::preempt() {
    changeState(ProcessState::Ready);
}

void Process::terminate(int now) {
    if (remainingTime_ != 0) {
        throw std::logic_error("El proceso " + id_ + " no puede terminar: le quedan " +
                               std::to_string(remainingTime_) + " unidades de ráfaga.");
    }
    changeState(ProcessState::Terminated);
    finishTime_ = now;
}

void Process::reset() noexcept {
    state_ = ProcessState::New;
    remainingTime_ = burstTime_;
    startTime_ = -1;
    finishTime_ = -1;
}
