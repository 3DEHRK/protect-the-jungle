@echo off

set SFML_INCLUDE_PATH=SFML-2.6.0\include
set SFML_LIB_PATH=SFML-2.6.0\lib

g++ -I"%SFML_INCLUDE_PATH%" -L"%SFML_LIB_PATH%" -o bin\app.exe main.cpp -lsfml-graphics -lsfml-system -lsfml-window

if errorlevel 1 (
    pause
) else (
    start bin\app.exe
)
