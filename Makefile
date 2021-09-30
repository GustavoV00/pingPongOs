# COMPILADOR
GCC = gcc

# Dirs
SRC=./src
INCLUDES=./includes

# Flags
CFLAGS=-Wall

# Files
TESTE=testafila.c
QUEUE=queue.c
RUN=queue
QUEUELB=queue.h

all: build
build: $(SRC)/testafila.o $(SRC)/queue.o
	$(GCC) $(CFLAGS) $(SRC)/testafila.o $(SRC)/queue.o -o queue

testafila.o: $(SRC)/$(TESTE) $(INCLUDES)/$(QUEUELB)
	$(GCC) -c $(SRC)/$(TESTE) $(CFLAGS) -o $(SRC)/testafila.o

queue.o: $(SRC)/$(QUEUE) $(INCLUDES)/$(QUEUELB)
	$(GCC) -c $(SRC)/$(QUEUE) $(CFLAGS) -o $(SRC)/queue.o

clean:
	rm -rf $(SRC)/*.o && rm queue
