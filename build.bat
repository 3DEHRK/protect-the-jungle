@echo off

set SFML_INCLUDE_PATH=SFML-2.6.1\include
set SFML_LIB_PATH=SFML-2.6.1\lib
set CURL_INCLUDE_PATH=curl-8.6.0_7-win32-mingw\include
set CURL_LIB_PATH=curl-8.6.0_7-win32-mingw\lib

g++ -I"%SFML_INCLUDE_PATH%" -I"%CURL_INCLUDE_PATH%" -L"%SFML_LIB_PATH%" -L"%CURL_LIB_PATH%" -o bin\app.exe main.cpp -lsfml-graphics -lsfml-system -lsfml-window -lsfml-audio -lcurl

if errorlevel 1 (
    pause
) else (
    start bin\app.exe
)
