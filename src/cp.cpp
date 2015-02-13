#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "Timer.h"
#include <unistd.h>

using namespace std;

typedef void (*fptr)(int &exitVal,const char *in, const char *out);
void copyBuf(int &exitVal,const char *in, const char *out);
void copyCharGet(int &exitVal,const char *in, const char *out);
void copyCharRW(int &exitVal,const char *in, const char *out);
void copyCharGen(int &exitVal,const char *in, const char *out, size_t bufLEN);

int main(int argc, char** argv)
{
	if(argc < 3 || argc > 4)
	{
		cout << "incorrect fuction signature: cp <SOURCE> <DESTINATION> <optional flag -t>\n";
		return 1;
	}
	int flag(0), exitVal;
	if(argc == 4)
		if(!strcmp(argv[3],"-t"))
			flag = 1;
	
	struct stat s;

	if(stat(argv[1],&s) == -1)
	{
		perror("input file doesn't exist");
		return 1;
	}
	if(stat(argv[2],&s) != -1)
	{
		cerr << "write file already exists\n";
		return 1;
	}

	char *in = argv[1], *out = argv[2];

	if(flag == 0 )
	{
		copyBuf(exitVal,in,out);
		return exitVal;
	}
	else
	{
		double eUT,eWC,eST;
		Timer t;
		fptr functions[3] = {&copyCharGet,&copyCharRW,&copyBuf};
		const char *dscp[3] = { "istream() & ostream() char by char \tRun times: \tUser: %.2f \tWallClock: %.2f \tSystem: %.2f\n",
								"read() & write(), buffer = 1 byte \tRun times: \tUser: %.2f \tWallClock: %.2f \tSystem: %.2f:\n",
								"read() & write(), buffer =  BUFSIZ \tRun times: \tUser: %.2f \tWallClock: %.2f \tSystem: %.2f:\n"};

		for(int i(0); i < 3; i++)
		{
			t.start();
			functions[i](exitVal,in,out);
			t.elapsedUserTime(eUT);
			t.elapsedWallclockTime(eWC);
			t.elapsedSystemTime(eST);
			printf(dscp[i],eUT,eWC,eST);
		}
		return exitVal;
	}
}

void copyCharGet(int &exitVal,const char *in, const char *out)
{
	ifstream fileIn(in);
	ofstream fileOut(out);

	char data = fileIn.get();
	while(!fileIn.eof())
	{
		fileOut.put(data);
		data = fileIn.get();
	}
	fileIn.close();
	fileOut.close();
}

void copyBuf(int &exitVal,const char *in, const char *out)
{
	size_t bufLEN = BUFSIZ;
	copyCharGen(exitVal,in,out,bufLEN);
}
void copyCharRW(int &exitVal,const char *in, const char *out)
{
	size_t bufLEN = 1;
	copyCharGen(exitVal,in,out,bufLEN);
}

void copyCharGen(int &exitVal,const char *in, const char *out, size_t bufLEN)
{
	int fileIn,fileOut;
	size_t inSize, outSize,bufSize = bufLEN;

	if(-1 == (fileIn = open(in,O_RDONLY)))
    {
        perror("error opening input file");
		exitVal = 1;
		return;
    }

    if(-1 == (fileOut = open(out, O_WRONLY |  O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))) // permissions 644
    {
        perror("error opening output file");
		exitVal = 1;
		return;
    }
    
    char *buf[bufSize];

    while((inSize = read(fileIn, &buf, bufSize)) > 0)
    {
        if((outSize = write(fileOut,&buf,(ssize_t) inSize)) == -1)
        {
            perror("write error");
			exitVal = 1;
			break;
        }
    }

    if(close(fileIn))
	{
		perror("error closing input file");
		exitVal = 1;
	}
	if(close(fileOut))
	{
		perror("error closing input file");
		exitVal = 1;
	}
}
