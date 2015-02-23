#include <cstdlib>
#include <iostream>
#include <queue>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sstream>
#include <string>
#include <vector>
#include <fcntl.h>

int numArgs = 50;
int numLen = 50;
char** array;

using namespace std;

string terminalPrefix();
void parse(string s, bool &run);
int sysCalls();
void init();
int sysCallsLimited(int fd[], int fd2, int instruction, char *argv[]);
void clear();
void nukem();
queue<string> split(string s);
void itemCount(string s);

void commands(vector<string> &token, bool &run);
void doPipe(vector<string> &token, bool &run, int index);
void doOr(vector<string> &token, bool &run, int index);
void doAnd(vector<string> &token, bool &run, int index); 
void doSemiColon(vector<string> &token, bool &run, int index);
int redirectAndRun(vector<string> &token, bool &run, int index);
int nextConnector(vector<string> &token);

int main()
{
   	string s,signature = terminalPrefix();// obtain username and hostname
	bool run = 1;
	
	while(run && cin)
	{
		cout.flush();
		cout << signature << " $ ";
		getline(cin, s);
		//parse(s,run);
		queue<string> list = split(s);
		
		vector<string> token;
		while(!list.empty())
		{
			token.push_back(list.front());
			list.pop();
		}
		
		int childPID = fork();
		if(childPID == -1) {
			perror("fork error");
			run = false;
		}
		if(childPID != 0) {
			int status = 0;
			waitpid(childPID, &status, 0);
		} else {
			run = 0;
			commands(token, run);
			exit(0);
		}
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

int sysCallsLimited(int fd[], int fd2[], int instruction, char *argv[])/////////////////////////////////////////////////////
{
	//instruction 0 = close(1), dup(fd[0])
	//instruction 1 = close(0), dup(fd[1]), close(1), dup(fd2[0])
	//instruction 2 = close(0), dup(fd2[1])

	if(instruction == 1)
	{
		close(0);
		dup(fd[1]);//close fd[1]?
		close(1);
		dup(fd2[0]);//close fd2[0]?
	}
	else if(instruction == 0)
	{
		close(1);
		dup(fd[0]);
	}
	else 
		if(instruction == 2)
		{
			close(0);
			dup(fd2[1]);
		}

	int pid;
	if ((pid=fork())== -1) // fork error
	{
        perror("fork");
		nukem();
		exit(1);
	}
	else if (pid == 0) //child - run process and error check
	{
		if (execvp(argv[0],argv) != 0)
		{
			perror(argv[0]);
			exit(1);
		}
	}else //parent
		return pid;
}

void redirect(vector<string> &vExtracted)
{
	int redirections = 0;
	for(int i(0); 1 < vExtracted.size(); i++)
		if(vExtracted[i].compare("|") == 0 || vExtracted[i].find(">") != string::npos || vExtracted[i].find("<") != string::npos)
			redirections++;
	//**********************************************************************************************************	
}

void commands(vector<string> &token, bool &run)
{
/*	if(token.size())//check if omit next command based on connector and exit status
	{
		if(token[0].compare("&&") || token[0].compare("||") || token[0].compare(";"))
		{
			wait(0);
			if((status == 0 && token[0].compare("||") == 0) || (status != 0 && token[0].compare("&&") == 0))
			{
				token.erase(token.begin());//erase connector
			}
			else
				if(token.size())
					token.erase(token.begin()); // erase connect or erase next command if previous if went off
		}
	}
	else
		return; // empty list
*/
	if(token.size())
	{
		//find next connector!
		int i = nextConnector(token);
		if( i < token.size() ){
			if( !token[i].compare("|") )
				doPipe(token, run,i);
			else if( !token[i].compare("||") )
				doOr(token, run,i);
			else if( !token[i].compare("&&") )
				doAnd(token,run,i);
			else if( !token[i].compare(";") )
				doSemiColon(token, run, i);
			else redirectAndRun(token,run,i);
		}else
			redirectAndRun(token,run,i);
		
	}
}

void doPipe(vector<string> &token, bool &run, int index)
{
	int fd[2];
	pipe(fd);

	int childPID = fork();
	if(childPID == -1)
	{
		perror("fork error");
		run = false;
	}
	if(childPID != 0) //parent process
	{
		dup2(fd[1],1);
		close(fd[0]);
		close(fd[1]);
		redirectAndRun(token,run,index);
		int status;
		//waitpid(childPID,&status,0);
	} else //child process
	{
		dup2(fd[0],0);
		close(fd[0]);
		close(fd[1]);
		for(int i = 0; i <= index; ++i)
			token.erase(token.begin());
		commands(token,run);
	}
}

void doOr(vector<string> & token, bool &run, int index)
{
	int status = redirectAndRun(token,run,index);

	if(status == 0)
		return;	
	
	for(int i = 0; i <= index; ++i)
		token.erase(token.begin());
	commands(token,run);
}

void doAnd(vector<string> &token, bool &run, int index)
{
	int status = redirectAndRun(token,run,index);

	if(status != 0)
		return;	
	
	for(int i = 0; i <= index; ++i)
		token.erase(token.begin());
	commands(token,run);
}

void doSemiColon(vector<string> &token, bool &run, int index)
{
	int status = redirectAndRun(token,run,index);

	for(int i = 0; i <= index; ++i)
		token.erase(token.begin());
	commands(token,run);
}

void doLeft1(vector<string> &token, bool &run, int index)
{
	int i = 0,fileIn;
	for(;i < token.size(); ++i)
		if(token[i].find("<") != string::npos)
			break;

	if(i < index)
	{
		if(!token[i].compare("<"))
			if(-1 == (fileIn = open(token[i+1].c_str(),O_RDONLY , 0644)))
    		{
        		perror("error opening output file");
       			run = false;
       			return;
    		}

		dup2(fileIn,0);
	}
}

void doRight(vector<string> &token, bool &run, int index)
{
	int i = 0,in = 1, out = 0,fileOut;
	bool boolFD = true;
	for(;i < token.size(); ++i)
		if(token[i].find(">") != string::npos)
			break;
	if(i < index)
	{
		if(isdigit(token[i][0])){
			char buf[2] = {token[i][0],0};
			out = atoi(buf);
		}
		if(token[i].size() > 2)
			if(isdigit(token[i][(token[i].size()-1)]) && token[i][token.size()-2] == '&')
			{
				char buf[2] = {token[i][token[i].size()-1],0};
				in = atoi(buf);	
				boolFD = false;
			}
		
		if(boolFD)
		{
			if(token[i].find(">>") != string::npos)
			{
					if(-1 == (fileOut = open(token[i+1].c_str(), O_WRONLY | O_APPEND |  O_CREAT, 0644)))
    				{
        				perror("error opening output file");
                		run = false;
                		return;
    				}
			}else{
				if(-1 == (fileOut = open(token[i+1].c_str(), O_WRONLY |  O_CREAT, 0644)))
    				{
        				perror("error opening output file");
						run = false;
        	        	return;
    				}
			}
			dup2(fileOut,in);
		}else{
			dup2(in,out);
		}
			
	}
}

int doLeft3(vector<string> &token, bool &run, int index)
{
	int i = 0;
	for(;i < token.size(); ++i)
		if(!token[i].compare("<<<"))
			return i;
	return i;
}

int redirectAndRun(vector<string> &token, bool &run, int index)
{
	//do redirection!
	doLeft1(token,run,index);
	int l3 = doLeft3(token,run,index);
	doRight(token,run,index);
	//...

	int fd[2] = {0,0};
	if(l3 < index)
	{
		pipe(fd);
	}
	//do execute command!
	int childPID = fork();
	if(childPID == -1)
	{
		perror("fork error");
		run = false;
	}
	if(childPID != 0)
	{
        if(l3 < index){
			close(fd[0]);
			int outSize = 0;
    	    if((outSize = write(fd[1],token[l3+1].c_str(),token[l3+1].size())) == -1)
        	{
				perror("write error");
				run = false;
				return -1;
			}
			close(fd[1]);
		}
		int status = 0;
		waitpid(childPID,&status,0);
		return status;
	} else
	{
		if(l3 < index) {
			dup2(fd[0],0);
			close(fd[0]);
			close(fd[1]);
		}
		
		int i(0);
		itemCount(token[0]);
		char *array[numArgs+1];
		for(int k = 0; k < numArgs; ++k){
			array[k] = new char[256];
			memset(array[k],0,256);
		}
		array[numArgs] = 0;		

		string test;
		stringstream ss;
		ss << token[0];
		while(ss >> test)// fill argv
		{
			strcpy(array[i],test.c_str());
			i++;
		}

		if (execvp(array[0],array) != 0){
			perror(array[0]);
			for(int k = 0; k < numArgs+1; ++k)
				delete[] array[k];
			exit(1);
		}
	}
}

void parse(string s, bool &run)
{
	queue<string> list;

	list = split(s);//fuction creates substrings of commands and separators

	vector<string> vList;
	while(!list.empty())
	{
		vList.push_back(list.front());
		list.pop();
	}

	int status(0);
	while(!vList.empty())
	{
		stringstream ss,ss2;
		string test;
		ss << vList[0];
		ss2 << vList[0];
		
		itemCount(vList[0]);
		
		test = vList[0];
		vList.erase(vList.begin());
		string next;

		if(!vList.empty())
			next = vList[0];


		if(next.compare("|") == 0 || next.find(">") != string::npos || next.find("<") != string::npos)	
			cout << "FOUND CHAIN\n"; //function to extract chain of redirectinons
		
		//process chain


/*
		// check if we have commands or connectors and depending on the connector check the previous exit status
		if(test == ";")
			continue;//continue means run the command
		else if( test.compare("||") == 0)
		{
			if(status == 0) // previous command succeeded (TRUE || command)
			{
				vList.erase(0);
				continue;
			}
			else
				continue; // previous command failed (FALSE || command)
		}
		else if(test.compare("&&") == 0)
		{
			if(status != 0)// previous command failed (FALSE && command)
			{
				vList.erase(0);
				continue;
			}
			else
				continue;// previous command succeeded (TRUE && command)
		}
		else //map of signals and commands
		{
			ss2 >> test;	
			if(test.compare("exit") == 0){
				run = 0;
				break;	
			}
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
*/	}
}

void init()// initializes char*[] or char ** array
{
	array = new char*[numArgs+1];
	
	for(int i(0); i < numArgs ; i++)
		array[i] = new char[numLen];
	array[numArgs] = 0;
	clear();
}

void clear()// clears the char*
{
	for(int i(0); i < numArgs ; i++)
		for(int j(0); j < numLen; j++)
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
	int len = s.size(), sPos(0),next2(0);
	for(int i(0); i < len; ++i)
	{
		char next(0),next2(0),previous(0);

		if(i+1 < len)
			next = s[i+1];
		else
			next = '.';

		if(i+2 < len)
			next2 = s[i+2];
		else
			next2 = '.';

		if(i > 0)
			previous = s[i-1];
		else
			previous = '.';

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
					if(i-sPos)
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
					if(i-sPos)
						list.push(s.substr(sPos,i-sPos));
					list.push("||");
					sPos = (i+2);
					i++;
				}
				else
				{
					if(i-sPos)
						list.push(s.substr(sPos,i-sPos));
					list.push("|");
					sPos = (i+1);

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
		case '<': // extract the single or tripple input redirection operator
			{
				if(next == '<' && next2 == '<') // append "<"
				{
					if(i-sPos)
						list.push(s.substr(sPos,i-sPos));
					list.push("<<<");
					sPos = (i+3);
					i+=2;
				}
				else // append "<"
				{
					if(i-sPos)
						list.push(s.substr(sPos,i-sPos));
					list.push("<");
					sPos = (i+1);
				}
			}
			break;
		case '>':// #> || #>&# || #>> || >> || > 
			{
				if(isdigit(previous)) // #> || #>&#
				{
					if(next == '&' && isdigit(next2)) // #>&#
					{
						if((i-1)-sPos)
							list.push(s.substr(sPos,(i-1)-sPos));
						list.push(s.substr(i-1,4));
						sPos = (i+3);
						i+=2;
					}
					else if(next == '>')// #>>
					{
						if((i-1)-sPos)
							list.push(s.substr(sPos,(i-1)-sPos));
						list.push(s.substr(i-1,3));
						sPos = (i+2);
						i++;
					}
					else // #>
					{
						if((i-1)-sPos)
							list.push(s.substr(sPos,(i-1)-sPos));
						list.push(s.substr((i-1),2));
						sPos = (i+1);
					}
					break;
				}
				else if(next == '>') // >>
				{
					if(i-sPos)
						list.push(s.substr(sPos,i-sPos));
					list.push(">>");
					sPos = (i+2);
					i++;
					break;
				}
				else // >
				{
					if(i-sPos)
						list.push(s.substr(sPos,i-sPos));
					list.push(">");
					sPos = (i+1);
				}
			}
		default:;
		}
	}
	if (sPos != len)
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

int nextConnector(vector<string> &token)
{
	int i = 0;
	for(;i<token.size();++i)
		if( !token[i].compare("|") || !token[i].compare("&&") || !token[i].compare("||") || !token[i].compare(";"))
			return i;
	return i;
}
