Write-Host "Compilando servidor DASH..."
gcc -Wall -Wextra -std=c11 -O2 -o servidor_dash.exe src\servidor_dash.c -lws2_32
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Compilando cliente DASH..."
gcc -Wall -Wextra -std=c11 -O2 -o cliente_dash.exe src\cliente_dash.c src\cliente_http.c src\parser_mpd.c -lws2_32
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Compilando cliente HTTP demo..."
gcc -Wall -Wextra -std=c11 -O2 -o cliente_http_demo.exe src\cliente_http_demo.c src\cliente_http.c -lws2_32
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "Build concluido."
Write-Host "Execute em dois terminais:"
Write-Host "  .\servidor_dash.exe"
Write-Host "  .\cliente_dash.exe localhost:8080 /stream.mpd"
