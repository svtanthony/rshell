#include <cstdlib>
#include <iostream>
#include <boost/tokenizer.hpp>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <string>

//#define numArgs 100 or base it on input
//#define numLen 50
//************************1) Dynamically build arrays in parse*******2) parsing ************ 3) check wait
int numArgs = 50;
int numLen = 50;
char** array;

using namespace std;

string terminalPrefix();
void parse(string s, bool &run);
void sysCalls();
void init();
void clear();
void nukem();
queue<string> split(string s);
void itemCount(stringstream ss);

int main()
{
	init();
   	string s,signature = terminalPrefix();
	bool run = 1;
	
	while(run)
	{
		cout.flush();
		cout << signature << " $ ";
		getline(cin, s);
		parse(s,run);
	}
	nukem();
	return 0;
}

string terminalPrefix()
{
	string signature;
	char *lgn,host[50];

	//get terminal variables and use them if possible	
	if((lgn = getlogin()) == NULL || gethostname(host,50) != 0)
	{
		perror("error obtaining credentials");
		signature = "";
	}
	else
		signature.append(lgn).append("@").append(host);

	return signature;
}

void sysCalls()
{
	int pid, status, status2;
	if ((pid=fork())== -1) // fork error
	{
        perror("fork");
		nukem();
		exit(1);
	}
	else if (pid == 0) //child - run process and error check
	{
		if (execvp(array[0],array) != 0)
		{
			perror(array[0]);
			exit(0);
		}
	}else //parent - wait for child before proceeding---------------------------------------------------------------------------
		if((status2 = waitpid(pid, &status, 0)) == -1)
		{
			perror("Waiting Error");
			nukem();
		//	cout << "status = " << status << " and status2 = " << status2 << endl;
			exit(1);
		}
		else
			//cout << "pid = " << pid << endl;//*****************+++++++++++++******************++++++++++++++++++++**************++++++++++++
}

void parse(string s, bool &run)
{
	queue<string> list;//fuctions creates substrings of commands and separators
	list = splt(s);
	while(!list.empty())
	{
		stringstream ss;
		string test;
		ss << list.front();
		list.pop();
		itemCount(ss);
		test = list.front();
			
		if(test == ";")
			continue;//continue means run the command
		else if( test == "||")
		{
			if(status == XYX) // previous command succeeded (TRUE || command)
			{
				list.pop();
				continue;
			}
			else
				continue; // previous command failed (FALSE || command)
		}
		else if(test == "&&")
		{
			if(status == hxjavxh)// previous command failed (FALSE && command)
			{
				list.pop();
				continue;
			}
			else
				continue;// previous command succeeded (TRUE && command)
		}
		else if(test == "exit")
		{
			run = 0;
			break;	
		}
		init();
		int i(0);
		while(ss >> test)
		{
			strcpy(array[i],s.c_str());
			i++;
		}
		syscalls();
		nukem();
	}
}

void init()
{
	array = new char*[numArgs];
	
	for(int i(0); i < numArgs ; i++)
		for(int j(0); j < numLen ; j++)
			array[i] = new char[numLen];
}

void clear()
{
	for(int i(0); i < numArgs ; i++)
		for(int j(0); j < numLen ; j++)
			array[i][j] = '\0';
}

void nukem()
{
	for(int i(numArgs-1); i < 0 ;i--)
		delete[] array[i];
	delete[] array;
}

queue<string> split(string s)
{
	queue<string> list;
	int len = s.size(), sPos(0);
	for(int i(0); i < len; ++i)
	{
		char next = 0;
		if(i+1 < len)
			next = s[i+1];
		switch(s[i])
		{
		case ';':
			{
				list.push(s.substr(sPos,i-sPos));
				list.push(";");
				sPos = (i+1);
			}
			break;
		case '&':
			{
				if(next == '&')
				{
					list.push(s.substr(sPos,i-sPos));
					list.push("&&");
					sPos = (i+2);
					i++;
				}
			}
			break;
		case '|':
			{
				if(next == '|')
				{
					list.push(s.substr(sPos,i-sPos));
					list.push("||");
					sPos = (i+2);
					i++;
				}
			}
			break;
		default:;
		}
	}
	list.push(s.substr(sPos));
	
	return list;
}

void itemCount(stringstream ss)
{
	queue<string > worker;
	string str;
	int size = 0;
	while(ss >> str)
	{
		worker.push(str);
		if(str.size() > size)
			size = str.size();
	}
	numArgs = worker.size();
	numLen = size;
}
