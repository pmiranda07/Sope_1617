CC = gcc
CFLAGS = -D_REENTRANT -lpthread -Wall -pthread

all: sauna gerador

Gerador: gerador.c
	$(CC) gerador.c -o gerador $(CFLAGS)

Parque: sauna.c
	$(CC) sauna.c -o sauna $(CFLAGS)

clean:
	rm sauna
	rm gerador