@echo off
rem ===========================================================================
rem  build.bat - Compila el motor, las pruebas y la demo con MSVC (C++17, x64)
rem
rem  Alternativa a CMake en Windows. Uso: abrir "x64 Native Tools Command
rem  Prompt for VS 2026" (lo instalan las Build Tools), entrar a esta carpeta
rem  y ejecutar:  build
rem  Los ejecutables quedan en la carpeta bin\
rem ===========================================================================
setlocal
cd /d "%~dp0"

where cl >nul 2>nul
if errorlevel 1 (
    echo [ERROR] No se encontro el compilador cl.exe.
    echo Abra "x64 Native Tools Command Prompt for VS" y vuelva a ejecutar build.bat.
    exit /b 1
)

if not exist bin mkdir bin
if not exist bin\obj mkdir bin\obj

set FLAGS=/nologo /std:c++17 /EHsc /W4 /permissive- /utf-8 /O2 /I. /Fobin\obj\
set CORE=Core\process.cpp Core\scheduler.cpp Core\algorithms\fcfs.cpp Core\algorithms\sjf.cpp

echo Compilando pruebas del motor...
cl %FLAGS% %CORE% tests\test_core.cpp /Febin\test_core.exe
if errorlevel 1 goto error

echo Compilando demo del motor...
cl %FLAGS% %CORE% demo\demo_motor.cpp /Febin\demo_motor.exe
if errorlevel 1 goto error

echo.
echo Listo. Ejecute bin\test_core.exe y bin\demo_motor.exe
exit /b 0

:error
echo.
echo [ERROR] La compilacion fallo. Revise los mensajes de arriba.
exit /b 1
