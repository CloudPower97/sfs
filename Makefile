SHELL=/bin/bash
CC=gcc
CFLAGS=-W -Wshadow -Wwrite-strings -ansi -Wcast-qual -Wall -Wextra -Wpedantic -pedantic -std=c99
SRC=src
OBJ=obj
BIN=bin
LIB=lib
LIBS= -lpthread

VPATH=$(BIN)

.PHONY: uninstall default all clean install

default: all

all : sfs

install : sfs
	@echo "Avrai bisogno dei permessi di root per poter installare questo programma sul tuo sistema"
	cp $(BIN)/sfs /usr/local/bin

sfs : $(SRC)/core/core.c $(LIB)/libsfs.a
	$(CC) $(CFLAGS) -o $(BIN)/$(@) $(<) -L $(LIB) -lsfs $(LIBS) -I $(SRC)/util/ -I $(SRC)/server/ -I $(SRC)/peer/

$(OBJ)/io.o : $(SRC)/io/io.c $(SRC)/io/io.h
	$(CC) $(CFLAGS) -o $(@) -c $(<) -I $(SRC)/peer/ -I $(SRC)/network/

$(OBJ)/peer.o : $(SRC)/peer/peer.c $(SRC)/peer/peer.h
	$(CC) $(CFLAGS) -o $(@) -c $(<)	-I $(SRC)/network/ -I $(SRC)/io/ -I $(SRC)/util/

$(OBJ)/server.o : $(SRC)/server/server.c $(SRC)/server/server.h
	$(CC) $(CFLAGS) -o $(@) -c $(<) -I $(SRC)/network/

$(OBJ)/network.o : $(SRC)/network/network.c $(SRC)/network/network.h
	$(CC) $(CFLAGS) -o $(@) -c $(<) -I $(SRC)/io/ -I $(SRC)/util/ -I $(SRC)/peer/

$(OBJ)/util.o : $(SRC)/util/util.c $(SRC)/util/util.h
	$(CC) $(CFLAGS) -o $(@) -c $(<)

$(LIB)/libsfs.a : $(OBJ)/util.o $(OBJ)/network.o $(OBJ)/io.o $(OBJ)/server.o $(OBJ)/peer.o
	ar -cr $(@) $(^)

clean :
	@$(RM) $(OBJ)/*
	@$(RM) $(BIN)/*
	@$(RM) $(LIB)/*

uninstall :
	@echo "Avrai bisogno dei permessi di root per poter rimuovere questo programma dal tuo sistema"
	@if [ -f "/usr/local/bin/sfs" ]; then $(RM) /usr/local/bin/sfs; fi
