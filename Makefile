CFLAGS = -Wall -Werror -pedantic -ansi
all: rshell ls
	
rshell: src/string.cpp bin 
	g++ src/string.cpp -o bin/rshell
bin:
	mkdir bin
ls: src/ls.cpp bin
	g++ -std=c++0x src/ls.cpp -o bin/ls

clean: bin
	rm -rf bin
