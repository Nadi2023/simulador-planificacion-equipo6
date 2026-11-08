// =============================================================================
//  Archivo     : tests/test_core.cpp
//  Proyecto    : Simulador de Algoritmos de Planificación de CPU
//  Responsable : Nadibeth Buiz (Arquitecta de Planificación) - Equipo 6
//
//  Descripción :
//    Pruebas automáticas del motor (Process, Scheduler) y de FCFS y SJF.
//    Los resultados esperados se calcularon a mano con los Conjuntos 1 y 2 del
//    enunciado (sección 5.1) y con casos especiales: CPU ociosa, llegadas
//    simultáneas, empates en SJF y entradas inválidas.
//
//    Uso: ejecutar test_core. Devuelve 0 si todo pasa y 1 si algo falla.
// =============================================================================
#include "Core/algorithms/fcfs.h"
#include "Core/algorithms/sjf.h"
#include "Core/process.h"
#include "Core/scheduler.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// -----------------------------------------------------------------------------
//  Mini marco de pruebas (sin librerías externas)
// -----------------------------------------------------------------------------
namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& what) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "    FALLA: " << what << "\n";
    }
}

void checkEq(long long got, long long expected, const std::string& what) {
    check(got == expected, what + " (esperado " + std::to_string(expected) +
                               ", obtenido " + std::to_string(got) + ")");
}

void checkNear(double got, double expected, const std::string& what) {
    check(std::fabs(got - expected) < 1e-9,
          what + " (esperado " + std::to_string(expected) + ", obtenido " +
              std::to_string(got) + ")");
}

template <typename Exception>
void checkThrows(const std::function<void()>& action, const std::string& what) {
    bool thrown = false;
    try {
        action();
    } catch (const Exception&) {
        thrown = true;
    } catch (...) {
    }
    check(thrown, what);
}

void runTest(const std::string& title, const std::function<void()>& test) {
    const int before = g_failures;
    try {
        test();
    } catch (const std::exception& e) {
        ++g_failures;
        std::cout << "    EXCEPCIÓN inesperada: " << e.what() << "\n";
    }
    std::cout << (g_failures == before ? "[ OK ]    " : "[FALLA]   ") << title << "\n";
}

// -----------------------------------------------------------------------------
//  Datos de prueba del enunciado
// -----------------------------------------------------------------------------
std::vector<Process> conjunto1() {
    return {Process("P1", 0, 8, 3), Process("P2", 1, 4, 1),
            Process("P3", 2, 9, 4), Process("P4", 3, 5, 2)};
}

std::vector<Process> conjunto2() {
    return {Process("P1", 0, 10, 2), Process("P2", 2, 3, 1), Process("P3", 4, 6, 3),
            Process("P4", 6, 1, 1), Process("P5", 8, 4, 2)};
}

// Cálculos de verificación (las métricas oficiales las implementa metrics.cpp).
double averageTurnaround(const ScheduleResult& r) {
    double sum = 0;
    for (const Process& p : r.processes) sum += p.finishTime() - p.arrivalTime();
    return sum / static_cast<double>(r.processes.size());
}

double averageWaiting(const ScheduleResult& r) {
    double sum = 0;
    for (const Process& p : r.processes) sum += p.finishTime() - p.arrivalTime() - p.burstTime();
    return sum / static_cast<double>(r.processes.size());
}

// Secuencia del Gantt en texto, por ejemplo "P1[0-8] P2[8-12]".
std::string ganttString(const ScheduleResult& r) {
    std::string s;
    for (const GanttSlice& g : r.gantt) {
        if (!s.empty()) s += ' ';
        s += g.processId + "[" + std::to_string(g.start) + "-" + std::to_string(g.end) + "]";
    }
    return s;
}

void checkTimes(const ScheduleResult& r, const std::string& id, int start, int finish) {
    const Process* p = r.findProcess(id);
    check(p != nullptr, id + " existe en el resultado");
    if (p == nullptr) return;
    checkEq(p->startTime(), start, id + " inicio");
    checkEq(p->finishTime(), finish, id + " fin");
}

// -----------------------------------------------------------------------------
//  Planificadores de prueba: verifican las herramientas que usarán Round Robin
//  y Prioridades, y que el motor detecte algoritmos mal escritos.
// -----------------------------------------------------------------------------

// Ejecuta 2 unidades del primero, lo desaloja, ejecuta completo el segundo y
// luego termina el primero. Simula lo que hará Round Robin.
class PreemptionProbe : public Scheduler {
public:
    std::string name() const override { return "Prueba de desalojo"; }
    bool isPreemptive() const override { return true; }

protected:
    void schedule() override {
        std::vector<Process*> ready = admitArrivals();  // ambos llegan en t = 0
        Process& a = *ready.at(0);
        Process& b = *ready.at(1);
        runProcess(a, 2);
        preempt(a);
        runProcess(b, b.remainingTime());
        runProcess(a, a.remainingTime());
    }
};

// Error típico: dejar la CPU ociosa con procesos listos.
class IdleWithReadyBug : public Scheduler {
public:
    std::string name() const override { return "Bug: ociosa con listos"; }
    bool isPreemptive() const override { return false; }

protected:
    void schedule() override {
        admitArrivals();
        idleUntilNextArrival();
    }
};

// Error típico: terminar el ciclo sin ejecutar a todos.
class UnfinishedBug : public Scheduler {
public:
    std::string name() const override { return "Bug: procesos sin terminar"; }
    bool isPreemptive() const override { return false; }

protected:
    void schedule() override {
        std::vector<Process*> ready = admitArrivals();
        runProcess(*ready.at(0), ready.at(0)->remainingTime());
    }
};

// Error típico: ejecutar otro proceso sin desalojar al que tiene la CPU.
class NoPreemptBug : public Scheduler {
public:
    std::string name() const override { return "Bug: sin desalojo"; }
    bool isPreemptive() const override { return true; }

protected:
    void schedule() override {
        std::vector<Process*> ready = admitArrivals();
        runProcess(*ready.at(0), 1);
        runProcess(*ready.at(1), 1);
    }
};

}  // namespace

// -----------------------------------------------------------------------------
//  Pruebas
// -----------------------------------------------------------------------------
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "Pruebas del motor de planificación (Process, Scheduler, FCFS, SJF)\n\n";

    runTest("FCFS - Conjunto 1 (inicio, fin, Gantt y promedios)", [] {
        FCFSScheduler fcfs;
        const ScheduleResult r = fcfs.run(conjunto1());
        check(ganttString(r) == "P1[0-8] P2[8-12] P3[12-21] P4[21-26]", "Gantt: " + ganttString(r));
        checkTimes(r, "P1", 0, 8);
        checkTimes(r, "P2", 8, 12);
        checkTimes(r, "P3", 12, 21);
        checkTimes(r, "P4", 21, 26);
        checkNear(averageTurnaround(r), 15.25, "retorno promedio");
        checkNear(averageWaiting(r), 8.75, "espera promedio");
        checkEq(r.totalTime, 26, "tiempo total");
        checkEq(r.busyTime, 26, "CPU ocupada");
        checkEq(r.idleTime, 0, "CPU ociosa");
        checkEq(r.contextSwitches, 3, "cambios de contexto");
    });

    runTest("SJF - Conjunto 1 (orden P1, P2, P4, P3)", [] {
        SJFScheduler sjf;
        const ScheduleResult r = sjf.run(conjunto1());
        check(ganttString(r) == "P1[0-8] P2[8-12] P4[12-17] P3[17-26]", "Gantt: " + ganttString(r));
        checkTimes(r, "P2", 8, 12);
        checkTimes(r, "P4", 12, 17);
        checkTimes(r, "P3", 17, 26);
        checkNear(averageTurnaround(r), 14.25, "retorno promedio");
        checkNear(averageWaiting(r), 7.75, "espera promedio");
    });

    runTest("FCFS - Conjunto 2", [] {
        FCFSScheduler fcfs;
        const ScheduleResult r = fcfs.run(conjunto2());
        check(ganttString(r) == "P1[0-10] P2[10-13] P3[13-19] P4[19-20] P5[20-24]",
              "Gantt: " + ganttString(r));
        checkNear(averageTurnaround(r), 13.2, "retorno promedio");
        checkNear(averageWaiting(r), 8.4, "espera promedio");
        checkEq(r.totalTime, 24, "tiempo total");
    });

    runTest("SJF - Conjunto 2 (orden P1, P4, P2, P5, P3)", [] {
        SJFScheduler sjf;
        const ScheduleResult r = sjf.run(conjunto2());
        check(ganttString(r) == "P1[0-10] P4[10-11] P2[11-14] P5[14-18] P3[18-24]",
              "Gantt: " + ganttString(r));
        checkNear(averageTurnaround(r), 11.4, "retorno promedio");
        checkNear(averageWaiting(r), 6.6, "espera promedio");
    });

    runTest("CPU ociosa al inicio y entre procesos", [] {
        FCFSScheduler fcfs;
        const ScheduleResult r = fcfs.run({Process("A", 2, 3), Process("B", 2, 3), Process("C", 10, 2)});
        check(ganttString(r) == "IDLE[0-2] A[2-5] B[5-8] IDLE[8-10] C[10-12]", "Gantt: " + ganttString(r));
        checkEq(r.busyTime, 8, "CPU ocupada");
        checkEq(r.idleTime, 4, "CPU ociosa");
        checkEq(r.totalTime, 12, "tiempo total");
    });

    runTest("Llegadas simultáneas: se respeta el orden de la lista", [] {
        FCFSScheduler fcfs;
        const ScheduleResult r = fcfs.run({Process("P3", 0, 2), Process("P1", 0, 2), Process("P2", 0, 2)});
        check(ganttString(r) == "P3[0-2] P1[2-4] P2[4-6]", "Gantt: " + ganttString(r));
        check(r.processes.at(0).id() == "P3", "el resultado conserva el orden de entrada");
    });

    runTest("SJF - empate en ráfaga: gana el que llegó primero", [] {
        SJFScheduler sjf;
        // C aparece antes que B en la lista, pero B llegó antes: debe ir B.
        const ScheduleResult r = sjf.run({Process("A", 0, 5), Process("C", 2, 2), Process("B", 1, 2)});
        check(ganttString(r) == "A[0-5] B[5-7] C[7-9]", "Gantt: " + ganttString(r));
    });

    runTest("SJF - empate total: gana el primero de la lista", [] {
        SJFScheduler sjf;
        const ScheduleResult r = sjf.run({Process("X", 0, 6), Process("Z", 1, 3), Process("Y", 1, 3)});
        check(ganttString(r) == "X[0-6] Z[6-9] Y[9-12]", "Gantt: " + ganttString(r));
    });

    runTest("SJF - CPU ociosa y llegada posterior", [] {
        SJFScheduler sjf;
        const ScheduleResult r = sjf.run({Process("P1", 3, 4), Process("P2", 3, 1), Process("P3", 20, 2)});
        check(ganttString(r) == "IDLE[0-3] P2[3-4] P1[4-8] IDLE[8-20] P3[20-22]", "Gantt: " + ganttString(r));
    });

    runTest("Un solo proceso", [] {
        SJFScheduler sjf;
        const ScheduleResult r = sjf.run({Process("Solo", 0, 7)});
        check(ganttString(r) == "Solo[0-7]", "Gantt: " + ganttString(r));
        checkEq(r.contextSwitches, 0, "cambios de contexto");
    });

    runTest("Traza de estados: Nuevo -> Listo -> Ejecutando -> Terminado", [] {
        FCFSScheduler fcfs;
        const ScheduleResult r = fcfs.run(conjunto1());
        checkEq(static_cast<long long>(r.events.size()), 12, "3 eventos por proceso");
        bool chronological = true;
        for (std::size_t i = 1; i < r.events.size(); ++i) {
            if (r.events[i].time < r.events[i - 1].time) chronological = false;
        }
        check(chronological, "eventos en orden cronológico");
        // P2 llega en t = 1 (Listo) y recibe la CPU en t = 8.
        int readyAt = -1, runningAt = -1;
        for (const StateEvent& e : r.events) {
            if (e.processId != "P2") continue;
            if (e.to == ProcessState::Ready) readyAt = e.time;
            if (e.to == ProcessState::Running) runningAt = e.time;
        }
        checkEq(readyAt, 1, "P2 pasa a Listo al llegar");
        checkEq(runningAt, 8, "P2 pasa a Ejecutando en t = 8");
        for (const Process& p : r.processes) {
            check(p.state() == ProcessState::Terminated, p.id() + " termina en estado Terminado");
        }
    });

    runTest("La lista original no se modifica y se puede volver a simular", [] {
        const std::vector<Process> original = conjunto1();
        SJFScheduler sjf;
        const ScheduleResult first = sjf.run(original);
        const ScheduleResult second = sjf.run(original);
        check(ganttString(first) == ganttString(second), "dos corridas dan el mismo Gantt");
        check(original.at(0).state() == ProcessState::New && original.at(0).finishTime() == -1,
              "la entrada sigue en estado Nuevo");
        // También funciona si se le pasa el resultado de otra simulación.
        FCFSScheduler fcfs;
        const ScheduleResult again = fcfs.run(first.processes);
        checkEq(again.totalTime, 26, "re-simular procesos ya terminados");
    });

    runTest("Validación de datos del proceso", [] {
        checkThrows<std::invalid_argument>([] { Process("P1", -1, 5); }, "llegada negativa");
        checkThrows<std::invalid_argument>([] { Process("P1", 0, 0); }, "ráfaga cero");
        checkThrows<std::invalid_argument>([] { Process("P1", 0, -3); }, "ráfaga negativa");
        checkThrows<std::invalid_argument>([] { Process("P1", 0, 5, -1); }, "prioridad negativa");
        checkThrows<std::invalid_argument>([] { Process("", 0, 5); }, "ID vacío");
        checkThrows<std::invalid_argument>([] { Process("P 1", 0, 5); }, "ID con espacio");
        check(Process::validate("P1", 0, 5, 1).empty(), "validate acepta datos correctos");
        check(!Process::validate("P1", 0, 0, 1).empty(), "validate explica el error");
    });

    runTest("Validación de la lista de procesos", [] {
        FCFSScheduler fcfs;
        checkThrows<std::invalid_argument>([&] { fcfs.run({}); }, "lista vacía");
        checkThrows<std::invalid_argument>(
            [&] { fcfs.run({Process("P1", 0, 2), Process("P1", 1, 3)}); }, "ID repetido");
        checkThrows<std::invalid_argument>(
            [&] { fcfs.run({Process("P1", 2000000000, 2), Process("P2", 0, 200000000)}); },
            "tiempos que desbordan un int");
    });

    runTest("Herramientas de desalojo (base para Round Robin y Prioridades)", [] {
        PreemptionProbe probe;
        const ScheduleResult r = probe.run({Process("A", 0, 5), Process("B", 0, 3)});
        check(ganttString(r) == "A[0-2] B[2-5] A[5-8]", "Gantt: " + ganttString(r));
        checkTimes(r, "A", 0, 8);  // el inicio es el PRIMER uso de CPU
        checkTimes(r, "B", 2, 5);
        checkEq(r.contextSwitches, 2, "cambios de contexto");
        bool sawPreemption = false;
        for (const StateEvent& e : r.events) {
            if (e.processId == "A" && e.from == ProcessState::Running && e.to == ProcessState::Ready) {
                sawPreemption = (e.time == 2);
            }
        }
        check(sawPreemption, "A pasa de Ejecutando a Listo en t = 2");
    });

    runTest("El motor detecta algoritmos con errores", [] {
        IdleWithReadyBug idleBug;
        checkThrows<std::logic_error>([&] { idleBug.run(conjunto1()); }, "CPU ociosa con listos");
        UnfinishedBug unfinished;
        checkThrows<std::logic_error>([&] { unfinished.run(conjunto1()); }, "procesos sin terminar");
        NoPreemptBug noPreempt;
        checkThrows<std::logic_error>(
            [&] { noPreempt.run({Process("A", 0, 3), Process("B", 0, 3)}); }, "ejecutar sin desalojar");
    });

    runTest("Transiciones válidas del modelo de estados", [] {
        check(isValidTransition(ProcessState::New, ProcessState::Ready), "Nuevo -> Listo");
        check(isValidTransition(ProcessState::Ready, ProcessState::Running), "Listo -> Ejecutando");
        check(isValidTransition(ProcessState::Running, ProcessState::Ready), "Ejecutando -> Listo");
        check(isValidTransition(ProcessState::Running, ProcessState::Terminated), "Ejecutando -> Terminado");
        check(!isValidTransition(ProcessState::New, ProcessState::Running), "Nuevo -> Ejecutando no");
        check(!isValidTransition(ProcessState::Terminated, ProcessState::Ready), "Terminado -> Listo no");
        check(toString(ProcessState::Running) == "Ejecutando", "nombre en español");
    });

    std::cout << "\n" << (g_checks - g_failures) << " de " << g_checks << " comprobaciones correctas.\n";
    if (g_failures == 0) {
        std::cout << "Todas las pruebas pasaron.\n";
        return 0;
    }
    std::cout << g_failures << " comprobaciones fallaron.\n";
    return 1;
}
