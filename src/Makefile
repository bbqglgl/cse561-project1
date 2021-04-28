CC = g++
OPT = -O3
#OPT = -g
WARN = -Wall
CFLAGS = $(OPT) $(WARN) #$(INC) $(LIB)

SRC = main.cpp
OBJ = main.o

#################################

all: scope

scope: $(OBJ)
	$(CC) -o main $(CFLAGS) $(OBJ)
	@echo "-----------DONE WITH MAIN-----------"
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc


clean:
	rm -f *.o main core Makefile.bak

clobber:
	rm -f *.o
