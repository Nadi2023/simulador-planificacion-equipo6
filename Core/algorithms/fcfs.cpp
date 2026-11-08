// =============================================================================
//  Archivo     : Core/algorithms/fcfs.cpp
//  Proyecto    : Simulador de Algoritmos de Planificación de CPU
//  Responsable : Nadibeth Buiz (Arquitecta de Planificación) - Equipo 6
//
//  Descripción :
//    Implementación de FCFS (First Come, First Served).
//
//  Estructura de datos: std::queue<Process*> como cola de listos (FIFO).
//
//  Algoritmo:
//    mientras queden procesos sin terminar:
//        1. pasar a la cola de listos los procesos que ya llegaron
//        2. si la cola está vacía, la CPU queda ociosa hasta la próxima llegada
//        3. si no, tomar el proceso del frente y ejecutarlo hasta que termine
//
//  Casos especiales:
//    - CPU ociosa: si nadie ha llegado, se registra un tramo IDLE en el Gantt.
//    - Llegadas simultáneas: se atienden en el orden de la lista de entrada.
//
//  Complejidad: O(n log n) por el ordenamiento inicial por llegada que hace el
//  motor; la simulación en sí es O(n).
// =============================================================================
#include "Core/algorithms/fcfs.h"

#include <queue>

void FCFSScheduler::schedule() {
    std::queue<Process*> readyQueue;  // cola de listos en orden de llegada

    while (!allFinished()) {
        // 1. Los que llegaron hasta ahora entran al final de la cola.
        for (Process* p : admitArrivals()) {
            readyQueue.push(p);
        }

        // 2. Nadie listo: la CPU espera a la próxima llegada.
        if (readyQueue.empty()) {
            idleUntilNextArrival();
            continue;
        }

        // 3. El primero de la cola usa la CPU hasta terminar (no apropiativo).
        Process* next = readyQueue.front();
        readyQueue.pop();
        runProcess(*next, next->remainingTime());
    }
}
