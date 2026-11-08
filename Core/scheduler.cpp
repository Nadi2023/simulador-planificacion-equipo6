// =============================================================================
//  Archivo     : Core/scheduler.cpp
//  Proyecto    : Simulador de Algoritmos de Planificación de CPU
//  Responsable : Nadibeth Buiz (Arquitecta de Planificación) - Equipo 6
//
//  Descripción :
//    Implementación del motor de planificación: preparación de la simulación,
//    herramientas comunes para los algoritmos (llegadas, ejecución, desalojo,
//    CPU ociosa), registro del diagrama de Gantt y de la traza de estados, y
//    verificación final del resultado.
// =============================================================================
#include "Core/scheduler.h"

#include <algorithm>
#include <climits>
#include <functional>
#include <numeric>
#include <set>
#include <stdexcept>

// -----------------------------------------------------------------------------
//  ScheduleResult
// -----------------------------------------------------------------------------
const Process* ScheduleResult::findProcess(const std::string& id) const noexcept {
    for (const Process& p : processes) {
        if (p.id() == id) {
            return &p;
        }
    }
    return nullptr;
}

// -----------------------------------------------------------------------------
//  run(): método plantilla que comparten los 4 algoritmos
// -----------------------------------------------------------------------------
ScheduleResult Scheduler::run(const std::vector<Process>& input) {
    validateInput(input);

    // 1. Copia de trabajo: la lista del usuario nunca se modifica.
    procs_ = input;
    for (Process& p : procs_) {
        p.reset();
    }

    // 2. Orden de llegada. stable_sort conserva el orden de la lista de entrada
    //    cuando dos procesos llegan en el mismo instante.
    arrivalOrder_.resize(procs_.size());
    std::iota(arrivalOrder_.begin(), arrivalOrder_.end(), std::size_t{0});
    std::stable_sort(arrivalOrder_.begin(), arrivalOrder_.end(),
                     [this](std::size_t a, std::size_t b) {
                         return procs_[a].arrivalTime() < procs_[b].arrivalTime();
                     });

    // 3. Estado inicial del motor.
    nextArrival_ = 0;
    running_ = nullptr;
    lastProcessOnCpu_.clear();
    clock_ = 0;
    contextSwitches_ = 0;
    gantt_.clear();
    events_.clear();

    // 4. El algoritmo concreto hace su trabajo.
    schedule();

    // 5. Control de calidad del resultado.
    verifyResult();

    // 6. Armado del resultado.
    ScheduleResult result;
    result.algorithm = name();
    result.processes = procs_;
    result.gantt = gantt_;
    result.events = events_;
    // Las llegadas se registran con su instante real, que puede ser anterior al
    // momento en que el algoritmo las admitió; por eso se ordena la traza.
    // stable_sort mantiene el orden lógico de los eventos del mismo instante.
    std::stable_sort(result.events.begin(), result.events.end(),
                     [](const StateEvent& a, const StateEvent& b) { return a.time < b.time; });
    result.totalTime = clock_;
    for (const GanttSlice& s : gantt_) {
        if (s.isIdle()) {
            result.idleTime += s.duration();
        } else {
            result.busyTime += s.duration();
        }
    }
    result.contextSwitches = contextSwitches_;
    return result;
}

// -----------------------------------------------------------------------------
//  Herramientas para los algoritmos
// -----------------------------------------------------------------------------
bool Scheduler::allFinished() const noexcept {
    return std::all_of(procs_.begin(), procs_.end(),
                       [](const Process& p) { return p.isFinished(); });
}

std::vector<Process*> Scheduler::admitArrivals() {
    std::vector<Process*> arrived;
    while (nextArrival_ < arrivalOrder_.size() &&
           procs_[arrivalOrder_[nextArrival_]].arrivalTime() <= clock_) {
        Process& p = procs_[arrivalOrder_[nextArrival_]];
        p.admit();
        // El proceso pasó a Listo en su instante de llegada.
        logEvent(p.arrivalTime(), p, ProcessState::New, ProcessState::Ready);
        arrived.push_back(&p);
        ++nextArrival_;
    }
    return arrived;
}

bool Scheduler::hasPendingArrivals() const noexcept {
    return nextArrival_ < arrivalOrder_.size();
}

int Scheduler::nextArrivalTime() const {
    if (!hasPendingArrivals()) {
        throw std::logic_error("No quedan procesos por llegar.");
    }
    return procs_[arrivalOrder_[nextArrival_]].arrivalTime();
}

void Scheduler::idleUntilNextArrival() {
    if (running_ != nullptr) {
        throw std::logic_error("La CPU no puede quedar ociosa: " + running_->id() +
                               " está en ejecución.");
    }
    for (const Process& p : procs_) {
        if (p.state() == ProcessState::Ready) {
            throw std::logic_error("La CPU no puede quedar ociosa: " + p.id() +
                                   " está listo para ejecutarse.");
        }
    }
    const int arrival = nextArrivalTime();
    if (arrival <= clock_) {
        throw std::logic_error("Hay procesos que ya llegaron y no se admitieron; "
                               "llame a admitArrivals() antes de idleUntilNextArrival().");
    }
    addSlice(GanttSlice::kIdleId, clock_, arrival);
    clock_ = arrival;
}

void Scheduler::runProcess(Process& p, int units) {
    // El proceso debe pertenecer a esta simulación (no a una copia externa).
    // std::less garantiza una comparación de punteros bien definida.
    const std::less<const Process*> before;
    if (procs_.empty() || before(&p, procs_.data()) ||
        !before(&p, procs_.data() + procs_.size())) {
        throw std::logic_error("runProcess() recibió un proceso que no pertenece a la simulación.");
    }
    if (running_ != nullptr && running_ != &p) {
        throw std::logic_error("No se puede ejecutar " + p.id() + " mientras " +
                               running_->id() + " tiene la CPU; desalójelo con preempt().");
    }
    if (units <= 0 || units > p.remainingTime()) {
        throw std::logic_error("No se pueden ejecutar " + std::to_string(units) +
                               " unidades de " + p.id() + ": le quedan " +
                               std::to_string(p.remainingTime()) + ".");
    }

    // Despacho: Listo -> Ejecutando.
    if (p.state() == ProcessState::Ready) {
        p.dispatch(clock_);
        logEvent(clock_, p, ProcessState::Ready, ProcessState::Running);
        running_ = &p;
        if (!lastProcessOnCpu_.empty() && lastProcessOnCpu_ != p.id()) {
            ++contextSwitches_;
        }
        lastProcessOnCpu_ = p.id();
    } else if (p.state() != ProcessState::Running) {
        throw std::logic_error("El proceso " + p.id() + " no puede ejecutarse: su estado es " +
                               toString(p.state()) + ".");
    }

    // Ejecución: avanza el reloj y queda registrada en el Gantt.
    const int start = clock_;
    p.execute(units);
    clock_ += units;
    addSlice(p.id(), start, clock_);

    // Fin de la ráfaga: Ejecutando -> Terminado.
    if (p.remainingTime() == 0) {
        p.terminate(clock_);
        logEvent(clock_, p, ProcessState::Running, ProcessState::Terminated);
        running_ = nullptr;
    }
}

void Scheduler::preempt(Process& p) {
    if (running_ != &p) {
        throw std::logic_error("No se puede desalojar a " + p.id() +
                               " porque no es el proceso en ejecución.");
    }
    p.preempt();
    logEvent(clock_, p, ProcessState::Running, ProcessState::Ready);
    running_ = nullptr;
}

// -----------------------------------------------------------------------------
//  Funciones internas
// -----------------------------------------------------------------------------
void Scheduler::validateInput(const std::vector<Process>& input) const {
    if (input.empty()) {
        throw std::invalid_argument("No hay procesos para simular. Agregue al menos uno.");
    }

    std::set<std::string> ids;
    long long lastArrival = 0;
    long long totalBurst = 0;
    for (const Process& p : input) {
        if (!ids.insert(p.id()).second) {
            throw std::invalid_argument("El ID \"" + p.id() +
                                        "\" está repetido; cada proceso debe tener un ID único.");
        }
        lastArrival = std::max(lastArrival, static_cast<long long>(p.arrivalTime()));
        totalBurst += p.burstTime();
    }

    // El último instante posible es la última llegada más todas las ráfagas.
    // Se calcula en long long para detectar si no cabe en un int.
    if (lastArrival + totalBurst > INT_MAX) {
        throw std::invalid_argument("Los tiempos de llegada y ráfaga son demasiado grandes "
                                    "para simular; use valores más pequeños.");
    }
}

void Scheduler::verifyResult() const {
    const std::string where = "Error interno en el algoritmo " + name() + ": ";

    if (running_ != nullptr) {
        throw std::logic_error(where + running_->id() + " quedó en ejecución al finalizar.");
    }
    for (const Process& p : procs_) {
        if (!p.isFinished()) {
            throw std::logic_error(where + "el proceso " + p.id() + " no terminó.");
        }
        if (p.startTime() < p.arrivalTime() || p.finishTime() - p.arrivalTime() < p.burstTime()) {
            throw std::logic_error(where + "los tiempos de " + p.id() + " son incoherentes.");
        }
    }

    // El Gantt debe ser continuo desde 0 hasta el final del reloj.
    int expectedStart = 0;
    for (const GanttSlice& s : gantt_) {
        if (s.start != expectedStart || s.duration() <= 0) {
            throw std::logic_error(where + "el diagrama de Gantt tiene huecos o tramos vacíos.");
        }
        expectedStart = s.end;
    }
    if (expectedStart != clock_) {
        throw std::logic_error(where + "el diagrama de Gantt no llega al final de la simulación.");
    }

    // Cada proceso debe aparecer en el Gantt exactamente su ráfaga.
    for (const Process& p : procs_) {
        int used = 0;
        for (const GanttSlice& s : gantt_) {
            if (s.processId == p.id()) {
                used += s.duration();
            }
        }
        if (used != p.burstTime()) {
            throw std::logic_error(where + p.id() + " usó la CPU " + std::to_string(used) +
                                   " unidades, pero su ráfaga es " +
                                   std::to_string(p.burstTime()) + ".");
        }
    }
}

void Scheduler::addSlice(const std::string& id, int start, int end) {
    // Si el mismo proceso sigue en la CPU sin interrupción, se alarga el tramo
    // anterior en lugar de crear uno nuevo (Gantt más limpio).
    if (!gantt_.empty() && gantt_.back().processId == id && gantt_.back().end == start) {
        gantt_.back().end = end;
        return;
    }
    gantt_.push_back(GanttSlice{id, start, end});
}

void Scheduler::logEvent(int time, const Process& p, ProcessState from, ProcessState to) {
    events_.push_back(StateEvent{time, p.id(), from, to});
}
