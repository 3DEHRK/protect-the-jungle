@echo off

set SFML_INCLUDE_PATH=SFML-2.6.0\include
set SFML_LIB_PATH=SFML-2.6.0\lib
set SOURCE_FILE=src\main.cpp
set OUTPUT_FILE=bin\app.exe

g++ -I"%SFML_INCLUDE_PATH%" -L"%SFML_LIB_PATH%" -o "%OUTPUT_FILE%" %SOURCE_FILE% -lsfml-graphics -lsfml-system -lsfml-window

if errorlevel 1 (
    pause
) else (
    start bin\app.exe
)
