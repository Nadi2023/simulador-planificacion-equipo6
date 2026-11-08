// =============================================================================
//  Archivo     : Core/process.h
//  Proyecto    : Simulador de Algoritmos de Planificación de CPU
//  Asignatura  : Sistemas Operativos - UCAB - Prof. Gustavo Lara Jr.
//  Responsable : Nadibeth Buiz (Arquitecta de Planificación) - Equipo 6
//  Estándar    : ISO C++17
//
//  Descripción :
//    Define la clase Process, que representa un proceso del simulador con los
//    datos que exige el enunciado (sección 3.2):
//      - ID único del proceso
//      - Tiempo de llegada (arrival time)
//      - Tiempo de ráfaga de CPU (burst time)
//      - Prioridad (para los algoritmos que la requieran)
//      - Estado actual (Nuevo, Listo, Ejecutando, Terminado)
//    Además guarda los datos que produce la simulación: tiempo restante,
//    instante de primera ejecución y de finalización.
//
//    Los datos de entrada se fijan al construir el objeto y no cambian. El
//    estado solo lo cambia el Scheduler (clase amiga), para que todas las
//    transiciones queden validadas y registradas en la traza de estados.
// =============================================================================
#pragma once

#include <string>

// -----------------------------------------------------------------------------
//  Estados de un proceso (modelo del enunciado, sección 3.2).
//
//  Transiciones válidas:
//      Nuevo ------> Listo          el proceso llega al sistema
//      Listo ------> Ejecutando     el planificador le asigna la CPU
//      Ejecutando -> Listo          desalojo (Round Robin, prioridad apropiativa)
//      Ejecutando -> Terminado      consumió toda su ráfaga
// -----------------------------------------------------------------------------
enum class ProcessState {
    New,        // Nuevo: todavía no llega (tiempo actual < llegada)
    Ready,      // Listo: llegó y espera la CPU en la cola de listos
    Running,    // Ejecutando: tiene la CPU
    Terminated  // Terminado: completó su ráfaga
};

// Nombre del estado en español, tal como lo pide el enunciado.
std::string toString(ProcessState state);

// Indica si el paso de un estado a otro está permitido por el modelo.
bool isValidTransition(ProcessState from, ProcessState to) noexcept;

class Scheduler;  // declaración adelantada: es la única clase que cambia estados

// -----------------------------------------------------------------------------
//  Clase Process
// -----------------------------------------------------------------------------
class Process {
public:
    // Crea un proceso validando sus datos. Lanza std::invalid_argument con un
    // mensaje claro si algún dato es inválido (ver Process::validate).
    //   id          : identificador único, por ejemplo "P1" (sin espacios)
    //   arrivalTime : instante de llegada, >= 0
    //   burstTime   : unidades de CPU que necesita, > 0
    //   priority    : nivel de prioridad, >= 0 (el equipo define si el número
    //                 menor es la prioridad más alta)
    Process(std::string id, int arrivalTime, int burstTime, int priority = 0);

    // Revisa los datos sin crear el objeto. Devuelve "" si son válidos o el
    // mensaje de error en caso contrario. Útil para file_handler y la interfaz,
    // que pueden avisar del error sin usar excepciones.
    static std::string validate(const std::string& id, int arrivalTime,
                                int burstTime, int priority);

    // --- Datos de entrada (no cambian durante la simulación) ---
    const std::string& id() const noexcept { return id_; }
    int arrivalTime() const noexcept { return arrivalTime_; }
    int burstTime() const noexcept { return burstTime_; }
    int priority() const noexcept { return priority_; }

    // --- Datos de la simulación ---
    ProcessState state() const noexcept { return state_; }
    int remainingTime() const noexcept { return remainingTime_; }

    // Instante en que el proceso usó la CPU por primera vez (-1 si aún no).
    // Es el dato que necesita el tiempo de respuesta.
    int startTime() const noexcept { return startTime_; }

    // Instante en que el proceso terminó (-1 si aún no ha terminado).
    // Es el dato que necesita el tiempo de retorno.
    int finishTime() const noexcept { return finishTime_; }

    bool hasStarted() const noexcept { return startTime_ >= 0; }
    bool isFinished() const noexcept { return state_ == ProcessState::Terminated; }

private:
    friend class Scheduler;

    // Transiciones de estado. Cada una verifica que el estado actual permita
    // el cambio y lanza std::logic_error si no (error de programación).
    void admit();              // Nuevo -> Listo
    void dispatch(int now);    // Listo -> Ejecutando (guarda startTime la 1.a vez)
    void execute(int units);   // consume 'units' de su tiempo restante
    void preempt();            // Ejecutando -> Listo
    void terminate(int now);   // Ejecutando -> Terminado (tiempo restante = 0)
    void reset() noexcept;     // vuelve a Nuevo para repetir la simulación

    void changeState(ProcessState next);

    // Datos de entrada
    std::string id_;
    int arrivalTime_;
    int burstTime_;
    int priority_;

    // Datos de la simulación
    ProcessState state_ = ProcessState::New;
    int remainingTime_;
    int startTime_ = -1;
    int finishTime_ = -1;
};
