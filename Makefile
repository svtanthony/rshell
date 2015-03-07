## Target source to compile for each command
MAIN_TARGETS = src/rshell.cpp src/ls.cpp src/cp.cpp

## Global header files
INCLUDE = 

## Compiler Flags
CFLAGS = -Wall -Werror -pedantic -ansi 
FLAGS = -std=c++11

# Compiler
CC = g++
all: rshell ls cp
	
rshell: src/rshell.cpp bin 
	$(CC) $(FLAGS) src/rshell.cpp -o bin/rshell

bin:
	mkdir bin

ls: src/ls.cpp bin
	$(CC) $(FLAGS) src/ls.cpp -o bin/ls

cp: src/cp.cpp src/Timer.h bin
	$(CC) $(FLAGS) src/cp.cpp -o bin/cp

clean: bin
	rm -rf bin

print:
	a2ps --font-size=8pts -E C++ --line-numbers=1 -M letter $(INCLUDE) $(MAIN_TARGETS) Makefile -o printout.ps
