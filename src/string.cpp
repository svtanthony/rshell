#include <cstdlib>
#include <iostream>
#include <queue>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <string>

int numArgs = 50;
int numLen = 50;
char** array;

using namespace std;

string terminalPrefix();
void parse(string s, bool &run);
int sysCalls();
void init();
void clear();
void nukem();
queue<string> split(string s);
void itemCount(string s);

int main()
{
	init();
   	string s,signature = terminalPrefix();// obtain username and hostname
	bool run = 1;
	
	while(run)
	{
		cout.flush();
		cout << signature << " $ ";
		getline(cin, s);
		parse(s,run);
	}
	return 0;
}

string terminalPrefix()//function to get username and hostname
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

int sysCalls()
{
	int pid, status;
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
			exit(1);
		}
	}else //parent - wait for child before proceeding
		if( waitpid(pid, &status, 0) == -1)
		{
			perror("Waiting Error");
			exit(1);
		}
	return status;
}

void parse(string s, bool &run)
{
	queue<string> list;
	list = split(s);//fuction creates substrings of commands and separators
	int status(0);
	while(!list.empty())
	{
		stringstream ss;
		string test;
		ss << list.front();
		itemCount(list.front());
		test = list.front();
		list.pop();
		// check if we have commands or connectors and depending on the connector check the previous exit status
		if(test == ";")
			continue;//continue means run the command
		else if( test.compare("||") == 0)
		{
			if(status == 0) // previous command succeeded (TRUE || command)
			{
				list.pop();
				continue;
			}
			else
				continue; // previous command failed (FALSE || command)
		}
		else if(test.compare("&&") == 0)
		{
			if(status != 0)// previous command failed (FALSE && command)
			{
				list.pop();
				continue;
			}
			else
				continue;// previous command succeeded (TRUE && command)
		}
		else if(test.compare("exit") == 0)
		{
			run = 0;
			break;	
		}
		init();
		int i(0);
		while(ss >> test)// fill the char** normally referred to as args to pass to execvp
		{
			strcpy(array[i],test.c_str());
			i++;
		}
		status = sysCalls();
		nukem();
	}
}

void init()// initializes char*[] or char ** array
{
	array = new char*[numArgs+1];
	
	for(int i(0); i < numArgs ; i++)
		for(int j(0); j < numLen ; j++)
			array[i] = new char[numLen];
	array[numArgs] = 0;
	clear();
}

void clear()// clears the char*
{
	for(int i(0); i < numArgs ; i++)
		for(int j(0); j < numLen ; j++)
			array[i][j] = '\0';
}

void nukem()//deletes the char* and the char**
{
	for(int i(numArgs-1); i >= 0 ;i--)
		delete[] array[i];
	delete[] array;
}

queue<string> split(string s)//check the connectors to create substrings
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
		case ';'://add the substring before the semicolon, then add a semilon to the queue
			{
				list.push(s.substr(sPos,i-sPos));
				list.push(";");
				sPos = (i+1);
			}
			break;
		case '&'://add the substring before && to the queue and the connector "&&"" 
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
		case '|'://add the substring before the || connector to the queue and the "||" connector to the queue
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
		case '#':// set position of iterator to end if string to ignore comments
			{
				list.push(s.substr(sPos, i-sPos));
				i = len;
				sPos = len;
			}
			break;
		default:;
		}
	}
	list.push(s.substr(sPos));//add the rest of the string to the queue
	
	return list;
}

void itemCount(string s)//set glocal variables for array creation
{
	stringstream ss;
	ss << s;
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
