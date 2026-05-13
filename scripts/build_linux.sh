#!/usr/bin/env bash
set -euo pipefail

gcc -Wall -Wextra -std=c11 -O2 -o servidor_dash src/servidor_dash.c
gcc -Wall -Wextra -std=c11 -O2 -o cliente_dash src/cliente_dash.c src/cliente_http.c src/parser_mpd.c
gcc -Wall -Wextra -std=c11 -O2 -o cliente_http_demo src/cliente_http_demo.c src/cliente_http.c

echo "Build concluido."
echo "Execute em dois terminais:"
echo "  ./servidor_dash"
echo "  ./cliente_dash localhost:8080 /stream.mpd"
