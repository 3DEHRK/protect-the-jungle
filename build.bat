@echo off

set SFML_INCLUDE_PATH=SFML-2.6.0\include
set SFML_LIB_PATH=SFML-2.6.0\lib

set SOURCE_FILE=main.cpp
set OUTPUT_FILE=app.exe

g++ -I"%SFML_INCLUDE_PATH%" -L"%SFML_LIB_PATH%" -o "%OUTPUT_FILE%" "%SOURCE_FILE%" -lsfml-graphics -lsfml-window -lsfml-system

start app.exe

pause