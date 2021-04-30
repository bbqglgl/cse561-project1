CC = g++
OPT = -O3
#OPT = -g
WARN = -Wall
CFLAGS = $(OPT) $(WARN) #$(INC) $(LIB)

SRC = ces561sim.cpp
OBJ = ces561sim.o
ONAME = ces561sim

#################################

all: scope

scope: $(OBJ)
	$(CC) -o $(ONAME) $(CFLAGS) $(OBJ)
	@echo "-----------DONE WITH MAIN-----------"
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc


clean:
	rm -f *.o main core Makefile.bak

clobber:
	rm -f *.o
