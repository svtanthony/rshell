CFLAGS = -Wall -Werror -pedantic -ansi
all: rshell ls cp
	
rshell: src/rshell.cpp bin 
	g++ src/rshell.cpp -o bin/rshell
bin:
	mkdir bin
ls: src/ls.cpp bin
	g++ -std=c++0x src/ls.cpp -o bin/ls
cp: src/cp.cpp src/Timer.h bin
	g++ src/cp.cpp -o bin/cp
clean: bin
	rm -rf bin
