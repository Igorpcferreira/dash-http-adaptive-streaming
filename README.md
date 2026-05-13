# DASH HTTP Adaptive Streaming

Projeto didático em C para demonstrar distribuição de conteúdo multimídia com HTTP e MPEG-DASH.

Autor: Igor Ferreira

## Objetivo

Executar, observar e descrever o funcionamento de um servidor HTTP/DASH e de um cliente DASH adaptativo. O cliente baixa o manifesto MPD, identifica as representações disponíveis e escolhe a qualidade dos segmentos com base no throughput medido durante a execução.

## Estrutura

```text
.
├── src/
│   ├── servidor_dash.c        # Servidor HTTP/DASH mínimo
│   ├── cliente_dash.c         # Cliente DASH com ABR por throughput
│   ├── cliente_http.c         # Biblioteca HTTP via sockets TCP
│   ├── cliente_http.h
│   ├── cliente_http_demo.c    # Cliente HTTP simples para testar o MPD
│   ├── parser_mpd.c           # Parser simplificado de manifesto MPD
│   ├── parser_mpd.h
│   └── net_compat.h           # Compatibilidade Windows/Linux para sockets
├── scripts/
│   ├── build_windows.bat
│   ├── build_windows.ps1
│   └── build_linux.sh
├── docs/
│   ├── exercicio_15.md
│   └── fluxo_dash.md
├── Makefile
└── README.md
```

## Compilação no Windows

Opção recomendada, sem depender da política de execução do PowerShell:

```powershell
.\scripts\build_windows.bat
```

Também é possível compilar manualmente:

```powershell
gcc -Wall -Wextra -std=c11 -O2 -o servidor_dash.exe src\servidor_dash.c -lws2_32
gcc -Wall -Wextra -std=c11 -O2 -o cliente_dash.exe src\cliente_dash.c src\cliente_http.c src\parser_mpd.c -lws2_32
gcc -Wall -Wextra -std=c11 -O2 -o cliente_http_demo.exe src\cliente_http_demo.c src\cliente_http.c -lws2_32
```

## Compilação no Linux/macOS

```bash
make
```

Ou:

```bash
./scripts/build_linux.sh
```

## Execução

Abra dois terminais na pasta do projeto.

Terminal 1:

```powershell
.\servidor_dash.exe
```

No Linux/macOS:

```bash
./servidor_dash
```

Terminal 2:

```powershell
.\cliente_dash.exe localhost:8080 /stream.mpd
```

No Linux/macOS:

```bash
./cliente_dash localhost:8080 /stream.mpd
```


Caso a porta `8080` esteja ocupada, execute o servidor em outra porta e informe a mesma porta no cliente:

```powershell
.\servidor_dash.exe 18080
.\cliente_dash.exe localhost:18080 /stream.mpd
```

## Fluxo implementado

1. O servidor sobe na porta `8080`.
2. O cliente solicita `GET /stream.mpd`.
3. O servidor retorna um manifesto MPD com três representações: 360p, 720p e 1080p.
4. O cliente analisa o MPD e inicia o download dos segmentos.
5. A cada segmento, o cliente mede o throughput real.
6. O algoritmo ABR escolhe a maior representação compatível com a banda estimada.
7. O servidor entrega segmentos sintéticos com variação de atraso para simular oscilação de rede.

## Evidências recomendadas para o relatório

- Print da compilação sem erros.
- Print do servidor ouvindo na porta `8080`.
- Print do cliente recebendo e interpretando o MPD.
- Print do cliente baixando segmentos e alternando representações.
- Print do servidor registrando os `GET` para `/stream.mpd` e `/seg_*_*.m4s`.

## Comandos Git

```bash
git init
git add .
git commit -m "Implementa simulacao DASH sobre HTTP"
git branch -M main
git remote add origin URL_DO_REPOSITORIO
git push -u origin main
```
