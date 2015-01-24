CFLAGS = -Wall -Werror -pedantic -ansi
all:bin
	g++ src/string.cpp -o bin/rshell
rshell:bin
	g++ src/string.cpp -o bin/rshell
bin:
	mkdir bin
