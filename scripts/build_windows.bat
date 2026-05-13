@echo off
echo Compilando servidor DASH...
gcc -Wall -Wextra -std=c11 -O2 -o servidor_dash.exe src\servidor_dash.c -lws2_32
if errorlevel 1 exit /b 1

echo Compilando cliente DASH...
gcc -Wall -Wextra -std=c11 -O2 -o cliente_dash.exe src\cliente_dash.c src\cliente_http.c src\parser_mpd.c -lws2_32
if errorlevel 1 exit /b 1

echo Compilando cliente HTTP demo...
gcc -Wall -Wextra -std=c11 -O2 -o cliente_http_demo.exe src\cliente_http_demo.c src\cliente_http.c -lws2_32
if errorlevel 1 exit /b 1

echo.
echo Build concluido.
echo Execute em dois terminais:
echo   servidor_dash.exe
echo   cliente_dash.exe localhost:8080 /stream.mpd
