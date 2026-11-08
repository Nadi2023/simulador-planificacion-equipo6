// =============================================================================
//  Archivo     : Core/scheduler.h
//  Proyecto    : Simulador de Algoritmos de Planificación de CPU
//  Asignatura  : Sistemas Operativos - UCAB - Prof. Gustavo Lara Jr.
//  Responsable : Nadibeth Buiz (Arquitecta de Planificación) - Equipo 6
//  Estándar    : ISO C++17
//
//  Descripción :
//    Motor principal de planificación. Define:
//      - GanttSlice     : un tramo del diagrama de Gantt (proceso o CPU ociosa).
//      - StateEvent     : un cambio de estado de un proceso, con su instante.
//      - ScheduleResult : todo lo que produce una simulación y que usan
//                         metrics.cpp (métricas) y results_display.cpp (Gantt).
//      - Scheduler      : clase base abstracta común a los 4 algoritmos.
//
//    Cómo funciona (patrón "método plantilla"):
//      1. run() valida los procesos, los copia y los pone en estado Nuevo.
//      2. run() llama a schedule(), que implementa cada algoritmo concreto.
//      3. schedule() usa las herramientas protegidas de esta clase
//         (admitArrivals, runProcess, preempt, idleUntilNextArrival) para
//         avanzar el reloj. Esas herramientas registran el Gantt y los
//         cambios de estado, así que ningún algoritmo tiene que hacerlo.
//      4. run() verifica que el resultado sea coherente (todos terminados,
//         Gantt continuo) y lo devuelve.
//
//    Convenciones de tiempo:
//      - El tiempo es discreto, en unidades enteras.
//      - El reloj empieza en 0. Si el primer proceso llega después de 0, el
//        Gantt comienza con un tramo de CPU ociosa (IDLE).
//      - Llegadas simultáneas: se atienden en el orden en que aparecen en la
//        lista de entrada (orden del archivo o del registro manual).
// =============================================================================
#pragma once

#include "Core/process.h"

#include <cstddef>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
//  Tramo del diagrama de Gantt: [start, end) usado por un proceso o ocioso.
// -----------------------------------------------------------------------------
struct GanttSlice {
    static constexpr const char* kIdleId = "IDLE";

    std::string processId;  // ID del proceso, o "IDLE" si la CPU estuvo ociosa
    int start = 0;          // instante de inicio (incluido)
    int end = 0;            // instante de fin (excluido)

    bool isIdle() const noexcept { return processId == kIdleId; }
    int duration() const noexcept { return end - start; }
};

// -----------------------------------------------------------------------------
//  Cambio de estado de un proceso (traza para mostrar o depurar).
// -----------------------------------------------------------------------------
struct StateEvent {
    int time = 0;
    std::string processId;
    ProcessState from = ProcessState::New;
    ProcessState to = ProcessState::New;
};

// -----------------------------------------------------------------------------
//  Resultado completo de una simulación.
// -----------------------------------------------------------------------------
struct ScheduleResult {
    std::string algorithm;              // nombre del algoritmo, ej. "FCFS"
    std::vector<Process> processes;     // mismo orden que la entrada, ya terminados
    std::vector<GanttSlice> gantt;      // secuencia de ejecución, desde t = 0
    std::vector<StateEvent> events;     // traza de estados en orden cronológico
    int totalTime = 0;                  // instante en que termina el último proceso
    int busyTime = 0;                   // unidades con la CPU ocupada (suma de ráfagas)
    int idleTime = 0;                   // unidades con la CPU ociosa
    int contextSwitches = 0;            // veces que la CPU pasó de un proceso a otro

    // Busca un proceso por su ID; devuelve nullptr si no existe.
    const Process* findProcess(const std::string& id) const noexcept;
};

// -----------------------------------------------------------------------------
//  Clase base de los algoritmos de planificación.
//
//  Para crear un algoritmo nuevo (Round Robin, Prioridades, SRTF):
//    1. Heredar de Scheduler.
//    2. Implementar name(), isPreemptive() y schedule().
//    3. Dentro de schedule(), repetir hasta que allFinished() sea verdadero:
//         - admitArrivals() para pasar a Listo lo que ya llegó,
//         - elegir un proceso y llamar runProcess(p, unidades),
//         - si el proceso no terminó y hay que sacarlo, llamar preempt(p),
//         - si no hay nadie listo, llamar idleUntilNextArrival().
//  Ver fcfs.cpp y sjf.cpp como ejemplos.
// -----------------------------------------------------------------------------
class Scheduler {
public:
    virtual ~Scheduler() = default;

    // Nombre corto del algoritmo (para tablas y gráficos).
    virtual std::string name() const = 0;

    // Descripción de una línea para la interfaz.
    virtual std::string description() const { return ""; }

    // true si el algoritmo puede quitarle la CPU a un proceso antes de que termine.
    virtual bool isPreemptive() const = 0;

    // Ejecuta la simulación completa sobre una copia de 'processes' (la lista
    // original no se modifica). Lanza std::invalid_argument si la entrada es
    // inválida (lista vacía, IDs repetidos, tiempos demasiado grandes) y
    // std::logic_error si el algoritmo deja el resultado incoherente.
    ScheduleResult run(const std::vector<Process>& processes);

protected:
    // Implementación del algoritmo concreto. Ver instrucciones arriba.
    virtual void schedule() = 0;

    // ---------------- Herramientas para los algoritmos ----------------

    // Instante actual del reloj de la simulación.
    int now() const noexcept { return clock_; }

    // true cuando todos los procesos están en estado Terminado.
    bool allFinished() const noexcept;

    // Pasa a Listo todos los procesos que llegaron hasta now() y que aún no se
    // habían admitido. Los devuelve en orden de llegada (empates: orden de la
    // lista de entrada) para que el algoritmo los agregue a su cola.
    std::vector<Process*> admitArrivals();

    // true si queda algún proceso que todavía no ha llegado.
    bool hasPendingArrivals() const noexcept;

    // Instante de la próxima llegada pendiente. Lanza std::logic_error si no hay.
    int nextArrivalTime() const;

    // Deja la CPU ociosa hasta la próxima llegada y avanza el reloj.
    // Solo se puede usar si no hay procesos Listos ni Ejecutando.
    void idleUntilNextArrival();

    // Le da la CPU a 'p' durante 'units' unidades (1 <= units <= tiempo restante):
    //   - si p estaba Listo, lo pasa a Ejecutando (y guarda su primer uso),
    //   - avanza el reloj y registra el tramo en el Gantt,
    //   - si p consumió toda su ráfaga, lo pasa a Terminado.
    // Si p no terminó, queda Ejecutando: el algoritmo decide si sigue con él
    // o lo desaloja con preempt(p).
    void runProcess(Process& p, int units);

    // Desaloja al proceso en ejecución: Ejecutando -> Listo. El algoritmo debe
    // volver a ponerlo en su cola (por ejemplo, al final en Round Robin).
    void preempt(Process& p);

    // Acceso de lectura a los procesos de la simulación en curso.
    const std::vector<Process>& processes() const noexcept { return procs_; }

private:
    void validateInput(const std::vector<Process>& input) const;
    void verifyResult() const;
    void addSlice(const std::string& id, int start, int end);
    void logEvent(int time, const Process& p, ProcessState from, ProcessState to);

    std::vector<Process> procs_;            // copia de trabajo de los procesos
    std::vector<std::size_t> arrivalOrder_; // índices de procs_ ordenados por llegada
    std::size_t nextArrival_ = 0;           // siguiente posición de arrivalOrder_ por admitir
    Process* running_ = nullptr;            // proceso que tiene la CPU (o nullptr)
    std::string lastProcessOnCpu_;          // para contar cambios de contexto
    int clock_ = 0;
    int contextSwitches_ = 0;
    std::vector<GanttSlice> gantt_;
    std::vector<StateEvent> events_;
};
