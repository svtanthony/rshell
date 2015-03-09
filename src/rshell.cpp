#include <algorithm>
#include <cstdlib>	
#include <cstring>
#include <dirent.h>		//directory entry
#include <errno.h>
#include <fcntl.h>
#include <iomanip>
#include <iostream>	
#include <grp.h>
#include <pwd.h>
#include <queue>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/ioctl.h>	//terminal size
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>


using namespace std;

void itemCount(string s,int &numArgs);
void doPipe(vector<string> &token, bool &run);
int doLeft3(vector<string> &token, bool &run, int index);
void doRight(vector<string> &token, bool &run, int index);
void doLeft1(vector<string> &token, bool &run, int index);
int redirectAndRun(vector<string> &token, bool &run);
int nextConnector(vector<string> &token);
void runCommands(vector<string> &v, bool &run);
int executeCMD(int argc, char *array[]);
string terminalPrefix();
string terminalDir();
vector<string> parse(string s);
void bg(int argc, char *array[]);
void fg(int argc, char *array[]);
void changeDir(int argc, char *array[]);
void exitTer(int argc, char *array[]);//*********************************************kill all processes, then exit
void handler(int sig)
{
	switch(sig)
	{
		case SIGINT:
		case SIGSTOP:
		default:
			break;
	}
}

int main()
{
	string s,signature = terminalPrefix(),workingDir = terminalDir();// obtain username and hostname
	bool run = 1;

	if(signal(SIGINT,handler) != 0 )
		perror("error replacing signal handler");

	while(run && cin)
	{
		workingDir = terminalDir();
		cout.flush();
		cout <<"\033[1;32m" << signature <<"\033[3;31m" <<  workingDir << " $ " << "\033[0m";
		getline(cin, s);
		vector<string> commands = parse(s);
		
		runCommands(commands,run);
		
	}
	return 0;
}

void runCommands(vector<string> &v,bool &run)
{
	int status(0);
	while(!v.empty())
	{
		//check connector vs command
		if(v.size() > 2){
			if(!v[1].compare("|"))
				doPipe(v,run);
		}
		else if (!v[0].compare("||"))
		{
			v.erase(v.begin());
			if(status == 0)
				v.erase(v.begin());
			continue;
		}
		else if (!v[0].compare("&&")){
			v.erase(v.begin());
			if(status != 0)
				v.erase(v.begin());
			continue;
		}
		else if(!v[0].compare(";")){
			v.erase(v.begin());
			continue;
		}

		string str(v[0]);
		if(str.find("exit") != string::npos || str.find("cd") != string::npos )
		{
			int i(0),numArgs;
			itemCount(v[0],numArgs);
			char *array[numArgs+1];
			for(int k = 0; k < numArgs; ++k){
				array[k] = new char[256];
				memset(array[k],0,256);
			}
			array[numArgs] = 0;

			string test;
			stringstream ss;
			ss << v[0];
			v.erase(v.begin());
			while(ss >> test)// fill argv
			{
				strcpy(array[i],test.c_str());
				i++;
			}
			if(!strcmp(array[0], "exit"))
				exitTer(numArgs,array);
			else
				changeDir(numArgs,array);
			continue;
		}	
		
		redirectAndRun(v,run);
		
					
	}
}

int nextConnector(vector<string> &token)
{
	int i = 0;
	for(;(unsigned int)i<token.size();++i)
		if( !token[i].compare("|") || !token[i].compare("&&") || !token[i].compare("||") || !token[i].compare(";"))
			return i;
	return i;
}

int redirectAndRun(vector<string> &token, bool &run)
{
	int index = nextConnector(token);
	//do redirection!
	doLeft1(token,run,index);
	int l3 = doLeft3(token,run,index);
	//doRight(token,run,index);
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
			int childID = fork();
			if(childID == -1) {perror("fork");}
			if(childID == 0){
				close(fd[0]);
				int outSize = 0;
    	   	 	if((outSize = write(fd[1],token[l3+1].c_str(),token[l3+1].size())) == -1)
        		{
					perror("write error");
					run = false;
					return -1;
				}
				close(fd[1]);
				exit(0);
			}
			if(childID != 0){
				close(fd[1]);
				close(fd[0]);
				cout << "erasing" << endl;
				token.erase(token.begin()+l3+1);//erase string
				token.erase(token.begin()+l3); // erase connector
			}
		}
		token.erase(token.begin());//erase child process
		int status = 0;
		waitpid(childPID,&status,0);
		return status;
	} else // child
	{
		if(l3 < index) {
			dup2(fd[0],0);
			close(fd[0]);
			close(fd[1]);
		}
		
		int i(0),numArgs;
		itemCount(token[0],numArgs);
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
		if (executeCMD(numArgs,array) != 0){
			for(int k = 0; k < numArgs+1; ++k)
				delete[] array[k];
			exit(1);
		}
	}
	return 1;
}

void doLeft1(vector<string> &token, bool &run, int index)
{
	int i = 0,fileIn;
	for(;(unsigned int)i < token.size(); ++i)
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
		token.erase(token.begin()+i+1);
		token.erase(token.begin()+i);

		dup2(fileIn,0);
	}
}

void doRight(vector<string> &token, bool &run, int index)
{
	int i = 0, in = 1, out = 0,fileOut;
	bool boolFD = true;
	for(;(unsigned int)i < token.size(); ++i)
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
				token.erase(token.begin()+i+1);
			}else{
				if(-1 == (fileOut = open(token[i+1].c_str(), O_WRONLY |  O_CREAT, 0644)))
    			{
        			perror("error opening output file");
					run = false;
        	       	return;
    			}
				token.erase(token.begin()+i+1);
			}
				dup2(fileOut,in);
			}
		}else{
			dup2(in,out);//stream out to be replaced by stream in
		}
		token.erase(token.begin()+i);		
		redirectAndRun(token,run);
	
		token.erase(token.begin());
		token.erase(token.begin());

}

int doLeft3(vector<string> &token, bool &run, int index)
{
	unsigned int i = 0;
	for(;i < token.size(); ++i)
		if(!token[i].compare("<<<"))
			return i;
	return i;
}

void doPipe(vector<string> &token, bool &run)// create fork for first command, then create a second fork for second command, modify fd as needed
{
	int fd[2];
	pipe(fd);

	int childPID = fork(),childPID2;
	if(childPID == -1)
	{
		perror("fork error");
		run = false;
	}
	if(childPID != 0) //parent process
	{
		token.erase(token.begin());//clear current command
		token.erase(token.begin());//clear connector

		// *******  second command execution fork # 2 *************************************************
		childPID2 = fork();
		if(childPID2 == -1) {
			perror("fork error");
			run = 0;
		}
		if(childPID2 == 0){
			dup2(fd[0],0);
			close(fd[0]);
			close(fd[1]);
			if(token.size() > 2)
				if(!(token[1].compare("|"))){//another pipe
					doPipe(token, run);
					exit(0);
				}
			redirectAndRun(token,run);
			exit(0);
		}

		// *********  fork # 2 end  ***********************************************************************
		close(fd[0]);
		close(fd[1]);
		int status,status2;
		waitpid(childPID,&status,0);
		waitpid(childPID2,&status2,0); 
		//return status2;
	} else //child process
	{
		dup2(fd[1],1);//change read in
		close(fd[0]);
		close(fd[1]);
		redirectAndRun(token,run);
		exit(0);
	}
}

int executeCMD(int argc, char *array[])//only child process should be here
{
/*	if(signal(SIGINT, SIG_DFL) != 0){
		perror("error restoting default signal");
		exit(1);
	}
*/
	const char *path;
	if((path = getenv("PATH")) == NULL){
		perror("error obtaining PATH");
		exit(1);
	}
	else{
		if(argc)//avoid dereference @ array[0]
		{
			int status;
			string s(path), str;
			stringstream ss;
			size_t found = s.find(":");
			while(found != string::npos)
			{
				s.replace(found,1," ");
				found = s.find(":");
			}
			
			if(argc > 0)
				status = execv(array[0],array);
	
			ss << s;
			while(ss >> str)
			{
				str.append("/");
				str.append(array[0]);
				status = execv(str.c_str(),array);
			}

			if(status == -1){
				perror(array[0]);
			//	exit(1);
			}
		}
	}
	return 1;
}

void changeDir(int argc, char *array[])
{
	if( argc > 1){
		if(chdir(array[1]) == -1){
			perror("bad path");
		}
	}
	else
	{
		const char *homedir;
		if ((homedir = getenv("HOME")) == NULL) {
			homedir = getpwuid(getuid())->pw_dir;
		}
		if(homedir == NULL)
			perror("error obtaining HOME var");
		else
			chdir(homedir);
	}
}

void exitTer(int argc, char *array[])//*********************************************kill all processes, then exit
{
	while (1) {
    	int status;
    	pid_t done = wait(&status);
    	if (done == -1) {
        	if (errno == ECHILD) break; // no more child processes
    	} else {
        	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            	perror("wait error");
           	 exit(1);
        	}
    	}
	}

	//kill all processes
	exit(0);
}

void fg(int argc, char *array[])
{
	;//^z
}

void bg(int argc, char *array[])
{
	;//^z
}

vector<string> parse(string s)
{
	vector<string> list;
	int len = s.size(), sPos(0) ;
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
				list.push_back(s.substr(sPos,i-sPos));
				list.push_back(";");
				sPos = (i+1);
			}
			break;
		case '&'://add the substring before && to the queue and the connector "&&"" 
			{
				if(next == '&')
				{
					if(i-sPos)
						list.push_back(s.substr(sPos,i-sPos));
					list.push_back("&&");
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
						list.push_back(s.substr(sPos,i-sPos));
					list.push_back("||");
					sPos = (i+2);
					i++;
				}
				else
				{
					if(i-sPos)
						list.push_back(s.substr(sPos,i-sPos));
					list.push_back("|");
					sPos = (i+1);

				}
			}
			break;
		case '#':// set position of iterator to end if string to ignore comments
			{
				list.push_back(s.substr(sPos, i-sPos));
				i = len;
				sPos = len;
			}
			break;
		case '<': // extract the single or tripple input redirection operator
			{
				if(next == '<' && next2 == '<') // append "<"
				{
					if(i-sPos)
						list.push_back(s.substr(sPos,i-sPos));
					list.push_back("<<<");
					sPos = (i+3);
					i+=2;
				}
				else // append "<"
				{
					if(i-sPos)
						list.push_back(s.substr(sPos,i-sPos));
					list.push_back("<");
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
							list.push_back(s.substr(sPos,(i-1)-sPos));
						list.push_back(s.substr(i-1,4));
						sPos = (i+3);
						i+=2;
					}
					else if(next == '>')// #>>
					{
						if((i-1)-sPos)
							list.push_back(s.substr(sPos,(i-1)-sPos));
						list.push_back(s.substr(i-1,3));
						sPos = (i+2);
						i++;
					}
					else // #>
					{
						if((i-1)-sPos)
							list.push_back(s.substr(sPos,(i-1)-sPos));
						list.push_back(s.substr((i-1),2));
						sPos = (i+1);
					}
					break;
				}
				else if(next == '>') // >>
				{
					if(i-sPos)
						list.push_back(s.substr(sPos,i-sPos));
					list.push_back(">>");
					sPos = (i+2);
					i++;
					break;
				}
				else // >
				{
					if(i-sPos)
						list.push_back(s.substr(sPos,i-sPos));
					list.push_back(">");
					sPos = (i+1);
				}
			}
		default:;
		}
	}
	if (sPos != len)
		list.push_back(s.substr(sPos));//add the rest of the string to the queue

	for(unsigned int i(0) ; i < list.size() ; ++i)
		while(!list[i].empty() && list[i][0] == ' ')
			list[i].erase(0,1);
				
	return list;
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

string terminalDir()
{
	char buf[PATH_MAX];
	const char *homedir;
	if(!getcwd(buf,PATH_MAX))
	{
		perror("can't read current working directory");
		return "";
	}

	//convert homePath to ~/
	if ((homedir = getenv("HOME")) == NULL) {
    	homedir = getpwuid(getuid())->pw_dir;
		if(homedir == NULL){
			perror("error obtataining HOME var");
			perror("error obtaining UID");
		}
	}
	
	string s(buf),t(homedir);

	if(!t.empty())
	{
		size_t found = s.find(t);
		if(found != string::npos)
			s.replace(0,t.size()," ~");
	}

	return s;
}

void itemCount(string s, int &numArgs)//set glocal variables for array creation
{
	stringstream ss;
	ss << s;
	queue<string > worker;
	string str;
	unsigned int size = 0;
	while(ss >> str)
	{
		worker.push(str);
		if(str.size() > size)
			size = str.size();
	}
	numArgs = worker.size();
}
