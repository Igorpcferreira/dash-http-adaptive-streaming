CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -pedantic -O2
SRC=src

ifeq ($(OS),Windows_NT)
  EXE=.exe
  LDLIBS=-lws2_32
else
  EXE=
  LDLIBS=
endif

all: servidor_dash$(EXE) cliente_dash$(EXE) cliente_http_demo$(EXE)

servidor_dash$(EXE): $(SRC)/servidor_dash.c $(SRC)/net_compat.h
	$(CC) $(CFLAGS) -o $@ $(SRC)/servidor_dash.c $(LDLIBS)

cliente_dash$(EXE): $(SRC)/cliente_dash.c $(SRC)/cliente_http.c $(SRC)/parser_mpd.c $(SRC)/cliente_http.h $(SRC)/parser_mpd.h $(SRC)/net_compat.h
	$(CC) $(CFLAGS) -o $@ $(SRC)/cliente_dash.c $(SRC)/cliente_http.c $(SRC)/parser_mpd.c $(LDLIBS)

cliente_http_demo$(EXE): $(SRC)/cliente_http_demo.c $(SRC)/cliente_http.c $(SRC)/cliente_http.h $(SRC)/net_compat.h
	$(CC) $(CFLAGS) -o $@ $(SRC)/cliente_http_demo.c $(SRC)/cliente_http.c $(LDLIBS)

clean:
	rm -f servidor_dash cliente_dash cliente_http_demo servidor_dash.exe cliente_dash.exe cliente_http_demo.exe
