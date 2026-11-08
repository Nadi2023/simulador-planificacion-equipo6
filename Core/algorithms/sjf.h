// =============================================================================
//  Archivo     : Core/algorithms/sjf.h
//  Proyecto    : Simulador de Algoritmos de Planificación de CPU
//  Responsable : Nadibeth Buiz (Arquitecta de Planificación) - Equipo 6
//
//  Descripción :
//    SJF (Shortest Job First) no apropiativo: el trabajo más corto primero.
//
//    Teoría (Tanenbaum y Bos, 2023, §2.5.2, pp. 160-161):
//      - Cuando la CPU queda libre, se elige, entre los procesos que ya
//        llegaron, el de menor tiempo de ejecución (ráfaga).
//      - El proceso elegido conserva la CPU hasta terminar.
//      - Supone que la duración de cada trabajo se conoce de antemano.
//      - Es óptimo para el tiempo de retorno promedio solo cuando todos los
//        trabajos están disponibles a la vez.
//      - Puede causar inanición: si siguen llegando trabajos cortos, uno
//        largo puede postergarse indefinidamente.
//
//    Desempates (acordados por el equipo):
//      1. menor ráfaga;
//      2. si las ráfagas son iguales, el que llegó primero;
//      3. si también llegaron juntos, el que aparece primero en la lista.
// =============================================================================
#pragma once

#include "Core/scheduler.h"

class SJFScheduler : public Scheduler {
public:
    std::string name() const override { return "SJF"; }
    std::string description() const override {
        return "El trabajo más corto primero (no apropiativo).";
    }
    bool isPreemptive() const override { return false; }

protected:
    void schedule() override;
};
