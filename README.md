# Simulador de Algoritmos de Planificación de CPU · Motor de planificación

**Sistemas Operativos · UCAB · Prof. Gustavo Lara Jr. · Equipo 6**
Módulo de la **Arquitecta de Planificación** (Nadibeth Buiz): `Process`, `Scheduler`, FCFS y SJF no apropiativo.
Plataforma: Windows 11 x64 (entrega y demostración) · C++17 · VS Code con CMake · MSVC Build Tools en Windows y Apple Clang en macOS.

---

## 1. Qué hay en esta carpeta

```
SimuladorPlanificacion/
├── CMakeLists.txt            Proyecto CMake (VS Code en Windows y macOS)
├── build.bat                 Alternativa en Windows sin CMake (MSVC)
├── .gitignore                Excluye ejecutables y carpetas de compilación
├── Core/
│   ├── process.h / .cpp      Clase Process y modelo de estados
│   ├── scheduler.h / .cpp    Motor: clase base Scheduler, Gantt, traza, validación
│   └── algorithms/
│       ├── fcfs.h / .cpp     First Come, First Served
│       └── sjf.h / .cpp      Shortest Job First (no apropiativo)
├── tests/test_core.cpp       17 pruebas automáticas (90 comprobaciones)
└── demo/demo_motor.cpp       Demo del hito H1: Conjuntos 1 y 2 con FCFS y SJF
```

Las carpetas `UI/`, `Utils/` y los archivos `round_robin`, `priority` y `metrics` los agrega cada integrante (ver la sección 5).

## 2. Cómo compilar

**En VS Code (Windows y macOS)**
1. Instalar las extensiones **C/C++** y **CMake Tools** de Microsoft.
2. *Archivo > Abrir carpeta…* y elegir `SimuladorPlanificacion`.
3. Cuando CMake Tools pida un kit (o con *CMake: Select a Kit*):
   - **Windows:** el de *Visual Studio Build Tools 2026* para **amd64 (x64)**.
   - **macOS:** el de **Clang** de las Command Line Tools de Xcode.
4. Compilar con **F7** (*CMake: Build*).
5. Elegir el ejecutable con *CMake: Set Launch/Debug Target* (`test_core` o `demo_motor`) y correrlo con **Shift+F5**, o depurarlo con **Ctrl+F5**.

**Por terminal**
- **Windows** (*x64 Native Tools Command Prompt for VS 2026*): ejecutar `build`. Los ejecutables quedan en `bin\`.
- **macOS:** `cmake -S . -B build && cmake --build build && ./build/test_core`

**Verificación hecha:** el código compila sin advertencias en modo estricto (`-Wall -Wextra -Wpedantic -Wconversion -Werror`) con GCC, con Clang usando libc++ (la misma biblioteca estándar de Apple Clang) y con MinGW para Windows, y pasa las pruebas con AddressSanitizer. Para MSVC ya vienen activadas `/W4 /permissive- /utf-8`. El anteproyecto pide compilar sin advertencias en MSVC y Apple Clang, así que la primera compilación en un equipo con Windows debe confirmarlo.

**Repositorio (responsabilidad de la Arquitecta):** esta carpeta es la base de `main`. Cada integrante trabaja en su rama, agrega sus `.cpp` al `CMakeLists.txt` y abre un pull request que revisa otro integrante. Antes de unirlo a `main`, se compila en Windows.

## 3. Resultados verificados

`test_core` debe terminar con **«90 de 90 comprobaciones correctas»**. Los valores esperados se calcularon a mano:

| Conjunto | Algoritmo | Orden de ejecución (Gantt) | Retorno prom. | Espera prom. |
|---|---|---|---|---|
| 1 | FCFS | P1 0-8 · P2 8-12 · P3 12-21 · P4 21-26 | 15,25 | 8,75 |
| 1 | SJF | P1 0-8 · P2 8-12 · P4 12-17 · P3 17-26 | 14,25 | 7,75 |
| 2 | FCFS | P1 0-10 · P2 10-13 · P3 13-19 · P4 19-20 · P5 20-24 | 13,20 | 8,40 |
| 2 | SJF | P1 0-10 · P4 10-11 · P2 11-14 · P5 14-18 · P3 18-24 | 11,40 | 6,60 |

Casos especiales probados: CPU ociosa al inicio y entre procesos, llegadas simultáneas, empates en SJF, un solo proceso, entradas inválidas, volver a simular la misma lista, y algoritmos mal escritos que el motor detecta.

## 4. Cómo funciona el motor

**`Process`** guarda los datos del enunciado (ID, llegada, ráfaga, prioridad, estado) y los que produce la simulación (tiempo restante, primer uso de CPU, fin). Los datos de entrada se validan al crear el objeto y no cambian después.

**Modelo de estados** (enunciado, sección 3.2):

```
Nuevo ──llega──> Listo ──despacho──> Ejecutando ──fin de ráfaga──> Terminado
                   ^                     │
                   └──── desalojo ───────┘   (Round Robin, prioridad apropiativa)
```

Solo el `Scheduler` puede cambiar el estado de un proceso, y cualquier transición inválida, como Terminado a Listo, se rechaza con un error. Cada cambio queda registrado con su instante en la traza (`events`).

**`Scheduler`** es la clase base de los 4 algoritmos (patrón *método plantilla*):

1. `run(lista)` valida la lista (no vacía, IDs únicos, tiempos que caben en un `int`), hace una copia y pone todos los procesos en estado Nuevo. La lista original nunca se modifica.
2. Llama a `schedule()`, que cada algoritmo implementa usando las herramientas protegidas del motor.
3. Revisa que el resultado sea coherente: todos terminados, Gantt continuo y cada proceso con exactamente su ráfaga. Si un algoritmo tiene un error, lo avisa con un mensaje claro en vez de mostrar resultados incorrectos.
4. Devuelve un `ScheduleResult`.

**Convenciones acordadas:**
- El reloj empieza en **0**. Si nadie ha llegado, el Gantt muestra un tramo `IDLE` (CPU ociosa).
- **Llegadas simultáneas:** se atienden en el orden de la lista de entrada (orden del archivo o del registro manual).
- **SJF:** gana la menor ráfaga; si empatan, el que llegó primero; si también llegaron juntos, el primero de la lista.

## 5. Contrato para el resto del equipo

### Lo que devuelve cada simulación: `ScheduleResult`

| Campo | Contenido |
|---|---|
| `algorithm` | Nombre del algoritmo (`"FCFS"`, `"SJF"`…) |
| `processes` | Procesos ya terminados, **en el mismo orden de la entrada** |
| `gantt` | `vector<GanttSlice>`: `processId` (o `"IDLE"`), `start`, `end`, `duration()`, `isIdle()` |
| `events` | Traza de estados: `time`, `processId`, `from`, `to` (`toString()` da el nombre en español) |
| `totalTime` | Instante en que termina el último proceso |
| `busyTime` / `idleTime` | Unidades con la CPU ocupada / ociosa |
| `contextSwitches` | Veces que la CPU pasó de un proceso a otro (métrica adicional opcional) |

De cada `Process`: `id()`, `arrivalTime()`, `burstTime()`, `priority()`, `startTime()` (primer uso de CPU), `finishTime()`, `state()`.

### Samuel · Estratega de Turnos (`round_robin.cpp`, `priority.cpp`)
Heredar de `Scheduler`, implementar `name()`, `isPreemptive()` y `schedule()`, y agregar el `.cpp` al `CMakeLists.txt`. Herramientas disponibles dentro de `schedule()`:

| Herramienta | Qué hace |
|---|---|
| `admitArrivals()` | Pasa a Listo lo que llegó hasta `now()` y lo devuelve en orden de llegada |
| `runProcess(p, u)` | Ejecuta `u` unidades de `p`: lo despacha si estaba Listo, avanza el reloj, registra el Gantt y lo termina si acabó su ráfaga |
| `preempt(p)` | Desaloja al proceso en ejecución (Ejecutando a Listo); luego hay que volver a ponerlo en la cola |
| `idleUntilNextArrival()` | CPU ociosa hasta la próxima llegada (solo si no hay nadie listo) |
| `now()`, `allFinished()`, `hasPendingArrivals()`, `nextArrivalTime()` | Consultas del estado de la simulación |

El motor ya cuenta los cambios de contexto (`contextSwitches`) cada vez que despacha a un proceso distinto, así que el registro de cambios de contexto que asigna el cronograma sale solo, sin código extra en Round Robin. `startTime()` se guarda solo en el **primer** despacho, así el tiempo de respuesta sale bien aunque el proceso se desaloje varias veces. En Round Robin, si un proceso llega en el mismo instante en que otro agota su quantum, llamar a `admitArrivals()` **antes** de reencolar al desalojado deja primero al que llega. Esa es la convención más usada, pero conviene acordarla con Paola para que sus cálculos en Excel coincidan. El motor lanza un error si se intenta ejecutar un proceso sin desalojar al que tiene la CPU, o si la CPU queda ociosa con procesos listos.

### Paola · Analista de Rendimiento (`metrics.cpp`)
Con `ScheduleResult` se calculan las 6 métricas del enunciado:
- **Retorno** = `finishTime() - arrivalTime()`
- **Espera** = retorno - `burstTime()`
- **Respuesta** = `startTime() - arrivalTime()`
- **Uso de CPU** = `busyTime / totalTime × 100`, con el reloj desde 0 (incluye la espera inicial si el primer proceso llega tarde)

### Franco · Diseñador de la Consola (`UI/`)
- `gantt` trae los tramos listos para dibujar, incluidos los `IDLE`.
- `description()` e `isPreemptive()` sirven para el menú de algoritmos.
- `run()` lanza `std::invalid_argument` con un mensaje en español cuando los datos están mal: atraparlo y mostrar `e.what()`.

### José · Guardián de Pruebas y Archivos (`Utils/`)
- Crear procesos con `Process(id, llegada, ráfaga, prioridad)`.
- Para avisar errores de una línea del archivo sin excepciones, usar `Process::validate(...)`: devuelve `""` si los datos son válidos o el mensaje del error.
- Los ID no pueden tener espacios, comas ni punto y coma.

## 6. Referencias

- Tanenbaum, A. S. y Bos, H. (2023). *Modern operating systems* (5.ª ed.). Pearson. §2.1.5 (estados de un proceso, p. 92) y §2.5.2 (FCFS y SJF, pp. 160-161).
- Silberschatz, A., Galvin, P. B. y Gagne, G. (2018). *Operating system concepts* (10.ª ed.). Wiley. Cap. 5.
- Lara Jr., G. (2026). *Enunciado del proyecto: Simulador de algoritmos de planificación de CPU con análisis de rendimiento*. UCAB.
