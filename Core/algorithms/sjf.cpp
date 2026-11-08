// =============================================================================
//  Archivo     : Core/algorithms/sjf.cpp
//  Proyecto    : Simulador de Algoritmos de Planificación de CPU
//  Responsable : Nadibeth Buiz (Arquitecta de Planificación) - Equipo 6
//
//  Descripción :
//    Implementación de SJF (Shortest Job First) no apropiativo.
//
//  Estructura de datos: std::vector<Process*> como conjunto de listos. Los
//  procesos se agregan en orden de llegada, así que el vector siempre queda
//  ordenado por llegada (y por orden de entrada en los empates).
//
//  Algoritmo:
//    mientras queden procesos sin terminar:
//        1. agregar a los listos los procesos que ya llegaron
//        2. si no hay listos, la CPU queda ociosa hasta la próxima llegada
//        3. si no, elegir el de menor ráfaga y ejecutarlo hasta que termine
//
//  Por qué std::min_element: devuelve el PRIMER elemento mínimo. Como el
//  vector está en orden de llegada, el empate total (misma ráfaga y misma
//  llegada) se resuelve solo a favor del que aparece primero en la entrada.
//
//  Complejidad: O(n^2) en el peor caso (n búsquedas lineales del mínimo).
//  Para los tamaños del simulador es más simple y claro que una cola de
//  prioridad, y da exactamente el mismo resultado.
// =============================================================================
#include "Core/algorithms/sjf.h"

#include <algorithm>
#include <vector>

namespace {

// Criterio de selección de SJF: menor ráfaga; si empatan, el que llegó antes.
bool isShorterJob(const Process* a, const Process* b) {
    if (a->burstTime() != b->burstTime()) {
        return a->burstTime() < b->burstTime();
    }
    return a->arrivalTime() < b->arrivalTime();
}

}  // namespace

void SJFScheduler::schedule() {
    std::vector<Process*> ready;  // procesos listos, en orden de llegada

    while (!allFinished()) {
        // 1. Los que llegaron hasta ahora pasan a estar listos.
        for (Process* p : admitArrivals()) {
            ready.push_back(p);
        }

        // 2. Nadie listo: la CPU espera a la próxima llegada.
        if (ready.empty()) {
            idleUntilNextArrival();
            continue;
        }

        // 3. Se elige el trabajo más corto y se ejecuta completo.
        auto shortest = std::min_element(ready.begin(), ready.end(), isShorterJob);
        Process* next = *shortest;
        ready.erase(shortest);
        runProcess(*next, next->remainingTime());
    }
}
