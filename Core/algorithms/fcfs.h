// =============================================================================
//  Archivo     : Core/algorithms/fcfs.h
//  Proyecto    : Simulador de Algoritmos de Planificación de CPU
//  Responsable : Nadibeth Buiz (Arquitecta de Planificación) - Equipo 6
//
//  Descripción :
//    FCFS (First Come, First Served): primero en llegar, primero en ser
//    atendido. No apropiativo.
//
//    Teoría (Tanenbaum y Bos, 2023, §2.5.2, p. 160):
//      - Hay una sola cola de procesos listos, ordenada por llegada.
//      - El proceso del frente usa la CPU hasta terminar su ráfaga; nunca se
//        le interrumpe por llevar mucho tiempo.
//      - Los procesos que llegan se colocan al final de la cola.
//      Ventaja: es el algoritmo más simple. Desventaja: un proceso largo
//      retrasa a todos los que llegan detrás de él.
// =============================================================================
#pragma once

#include "Core/scheduler.h"

class FCFSScheduler : public Scheduler {
public:
    std::string name() const override { return "FCFS"; }
    std::string description() const override {
        return "Primero en llegar, primero en ser atendido (no apropiativo).";
    }
    bool isPreemptive() const override { return false; }

protected:
    void schedule() override;
};
