// =============================================================================
//  Archivo     : demo/demo_motor.cpp
//  Proyecto    : Simulador de Algoritmos de Planificación de CPU
//  Responsable : Nadibeth Buiz (Arquitecta de Planificación) - Equipo 6
//
//  Descripción :
//    Demostración del hito H1 (motor + FCFS) y de SJF. Corre los Conjuntos 1 y
//    2 del enunciado y muestra en consola el diagrama de Gantt, los tiempos de
//    cada proceso y la traza de estados.
//
//    Es solo una herramienta de verificación para el equipo: la interfaz
//    oficial está en UI/ y las métricas oficiales en Core/metrics.cpp.
// =============================================================================
#include "Core/algorithms/fcfs.h"
#include "Core/algorithms/sjf.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

// Gantt textual: cada tramo ocupa 2 caracteres por unidad de tiempo.
void printGantt(const ScheduleResult& r) {
    std::string bar = "|";
    std::string axis;
    for (const GanttSlice& s : r.gantt) {
        const std::string label = s.isIdle() ? "--" : s.processId;
        const int width = std::max(static_cast<int>(label.size()) + 2, s.duration() * 2);
        const int left = (width - static_cast<int>(label.size())) / 2;
        bar += std::string(static_cast<std::size_t>(left), ' ') + label +
               std::string(static_cast<std::size_t>(width - left) - label.size(), ' ') + "|";

        std::string mark = std::to_string(s.start);
        axis += mark + std::string(static_cast<std::size_t>(width + 1) - mark.size(), ' ');
    }
    axis += std::to_string(r.totalTime);
    std::cout << "  " << bar << "\n  " << axis << "\n";
}

void printTable(const ScheduleResult& r) {
    std::cout << "  " << std::left << std::setw(8) << "Proceso" << std::right << std::setw(8)
              << "Llegada" << std::setw(8) << "Ráfaga" << std::setw(8) << "Inicio" << std::setw(6)
              << "Fin" << std::setw(9) << "Retorno" << std::setw(8) << "Espera" << std::setw(11)
              << "Respuesta" << "\n";
    double sumTurnaround = 0;
    double sumWaiting = 0;
    for (const Process& p : r.processes) {
        const int turnaround = p.finishTime() - p.arrivalTime();
        const int waiting = turnaround - p.burstTime();
        const int response = p.startTime() - p.arrivalTime();
        sumTurnaround += turnaround;
        sumWaiting += waiting;
        // "Ráfaga" tiene una tilde: se compensa un espacio en la columna.
        std::cout << "  " << std::left << std::setw(8) << p.id() << std::right << std::setw(8)
                  << p.arrivalTime() << std::setw(7) << p.burstTime() << std::setw(8)
                  << p.startTime() << std::setw(6) << p.finishTime() << std::setw(9) << turnaround
                  << std::setw(8) << waiting << std::setw(11) << response << "\n";
    }
    const double n = static_cast<double>(r.processes.size());
    std::cout << std::fixed << std::setprecision(2)
              << "  Promedios -> retorno: " << sumTurnaround / n
              << " | espera: " << sumWaiting / n << " | CPU ocupada: " << r.busyTime << " de "
              << r.totalTime << " unidades\n";
}

void simulate(Scheduler& scheduler, const std::string& setName, const std::vector<Process>& set) {
    const ScheduleResult r = scheduler.run(set);
    std::cout << "\n=== " << r.algorithm << " - " << setName << " ===\n";
    std::cout << "  " << scheduler.description() << "\n\n";
    printGantt(r);
    std::cout << "\n";
    printTable(r);
}

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);  // para mostrar tildes en la consola de Windows
#endif
    const std::vector<Process> conjunto1 = {Process("P1", 0, 8, 3), Process("P2", 1, 4, 1),
                                            Process("P3", 2, 9, 4), Process("P4", 3, 5, 2)};
    const std::vector<Process> conjunto2 = {Process("P1", 0, 10, 2), Process("P2", 2, 3, 1),
                                            Process("P3", 4, 6, 3), Process("P4", 6, 1, 1),
                                            Process("P5", 8, 4, 2)};
    try {
        FCFSScheduler fcfs;
        SJFScheduler sjf;
        simulate(fcfs, "Conjunto 1", conjunto1);
        simulate(sjf, "Conjunto 1", conjunto1);
        simulate(fcfs, "Conjunto 2", conjunto2);
        simulate(sjf, "Conjunto 2", conjunto2);

        // Traza de estados de FCFS con el Conjunto 1.
        const ScheduleResult trace = fcfs.run(conjunto1);
        std::cout << "\n=== Traza de estados (FCFS - Conjunto 1) ===\n";
        for (const StateEvent& e : trace.events) {
            std::cout << "  t = " << std::setw(2) << e.time << "  " << e.processId << ": "
                      << toString(e.from) << " -> " << toString(e.to) << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
