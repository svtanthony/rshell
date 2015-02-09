#include <algorithm>
#include <cstdlib>	
#include <cstring>
#include <dirent.h>		//directory entry
#include <errno.h>
#include <iomanip>
#include <iostream>	
#include <grp.h>
#include <pwd.h>
#include <queue>
#include <stdio.h>
#include <string>
#include <sys/ioctl.h>	//terminal size
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

using namespace std;

int flag = 0;
int exitVal = 0;

#define a_flag 01
#define l_flag 02
#define R_flag 04
#define m_dir_flag 010

vector<string> variables(int argc,char **argv);
int winSize();
void theFunction(vector<string> results, string s);
void printVector(const vector<string>& results);
bool comparison(string first, string second);

int main(int argc, char *argv[])
{
	vector<string> path = variables(argc,argv);//set flags and obtain path variables
	vector<string> results;//used for printing

	if(path.empty())
		path.push_back(".");
	
	if(path.size() > 1)
		sort(path.begin(),path.end(),comparison);

	if(path.size()){
		char buf[PATH_MAX];
		if(!getcwd(buf,PATH_MAX))
			 perror("Cannot read current working directory");
		for(int i(0) ; i < path.size(); i++) {//use for file processing
			int result = chdir(path[i].c_str());
			if(result!=0){
				theFunction(results,path[i]);
			}else{
				theFunction(results,".");
				chdir(buf);
			}
		}
	}
	//printVector(results);
	//results.erase(results.begin(),results.end());

	//use for directory processing

	return exitVal;
}
vector<string> variables(int argc,char **argv)//sets flags and determines paths
{	
	vector<string> pathVar;	

	for(int i(1); i < argc ; i++)
	if(argv[i][0] == '-' && argv[i][1] != '\0'){//flag passed in, extract all flags
		for(int j(1); argv[i][j] != '\0'; j++)
			switch(argv[i][j])
			{
			case 'a':
				flag = flag | a_flag;
				break;
			case 'l':
				flag = flag | l_flag;
				break;
			case 'R':
				flag = flag | R_flag | m_dir_flag;
				break;
			default:
				cout << "invalid option -- '" << argv[i][j] << "'\n"
				<< "The available flags are {a,l,R}\n";
				exit(1);
			}
	}
	else //path was given
	{
		pathVar.push_back(argv[i]);
	}
	return pathVar;
}

int winSize()//obtain terminal size
{
    struct winsize w;
	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)){
		perror("error obtaining window size");
		return 80;
	}
	else
		return w.ws_col;
}


void theFunction(vector<string> results, string s)
{
	vector<string> subDir(0);//used for recursive traversal
	DIR *pDir;
	struct dirent *file;
	struct stat fileinfo;
	struct passwd *pwd;
	struct group *grp;
	struct tm date;

	string s2;
	/*char buf[PATH_MAX];
	getcwd(buf,PATH_MAX);
	string cwd = buf;	*/
	if(stat(s.c_str(),&fileinfo))//bad path
	{
		s2 = "cannot access ";
		s2 += s;
		perror(s2.c_str());
	}
	else // valid path
	{
		if(!S_ISDIR(fileinfo.st_mode))//working with file
		{
			if(flag & l_flag)
			{
				if(!localtime_r(&fileinfo.st_mtime, &date)){
					perror("time data error");
				}

				char timebuf[100];

				if(!strftime(timebuf,sizeof(timebuf), "%h %e %R",&date)){
					perror("time converstion error");
				}
				
				string user, group;
				string combine = ((S_ISDIR(fileinfo.st_mode)) ? "d" : (S_ISLNK(fileinfo.st_mode)) ? "l" : "-");
				combine.append(((fileinfo.st_mode & S_IRUSR) ? "r" : "-"));
				combine.append(((fileinfo.st_mode & S_IWUSR) ? "w" : "-"));
				combine.append(((fileinfo.st_mode & S_IXUSR) ? "x" : "-"));	
				combine.append(((fileinfo.st_mode & S_IRGRP) ? "r" : "-"));
				combine.append(((fileinfo.st_mode & S_IWGRP) ? "w" : "-"));
				combine.append(((fileinfo.st_mode & S_IXGRP) ? "x" : "-"));
				combine.append(((fileinfo.st_mode & S_IROTH) ? "r" : "-"));
				combine.append(((fileinfo.st_mode & S_IWOTH) ? "w" : "-"));
				combine.append(((fileinfo.st_mode & S_IXOTH) ? "x" : "-"));

				if(pwd = getpwuid(fileinfo.st_uid))
					user = pwd->pw_name;
				else{
					user = "unknown";
					perror("error obtaining user name");
				}

				if(grp = getgrgid(fileinfo.st_gid))
					group = grp->gr_name;
				else{
					group = "unknown";
					perror("error obtaining group name");
				}

				
				results.push_back(combine);				//0
				results.push_back(std::to_string(fileinfo.st_nlink));	//1
				results.push_back(user);				//2
				results.push_back(group);				//3
				results.push_back(std::to_string(fileinfo.st_size));	//4
				results.push_back(timebuf);				//5
				results.push_back(s);					//6

			}
			else
				results.push_back(s);
			printVector(results);
		}
		else //working with directory
		{
			if(!(pDir = opendir(s.c_str()))){
				s2 = "cannot access ";
				s2 += s;
				perror(s2.c_str());
				exitVal = 1; 
			}
			else
			{
				vector<string> tmp;
				while(file = readdir(pDir))
					tmp.push_back(file->d_name);
				sort(tmp.begin(),tmp.end(),comparison);
				struct { const char* d_name;} filea, *file=&filea;
				for(int f(0); f < tmp.size(); ++f)
				{
					file->d_name = tmp[f].c_str();

					if(stat(file->d_name,&fileinfo))//bad path
					{
						s2 = "cannot access ";
						s2 += s;
						perror(s2.c_str());
					}
					else
					{
						if(file->d_name[0] == '.' && !(flag & a_flag))
							continue;
						if((S_ISDIR(fileinfo.st_mode)) && (flag & R_flag) && strcmp(file->d_name,".") && strcmp(file->d_name,".."))
							subDir.push_back(/*s.append("/").append(file->d_name)*/file->d_name);
						if(!(flag & l_flag))
							results.push_back(file->d_name);
						else
						{
							if(!localtime_r(&fileinfo.st_mtime, &date)){
								perror("time data error");
 							}

							char timebuf[100];

							if(!strftime(timebuf,sizeof(timebuf), "%h %e %R",&date)){
								 perror("time converstion error");
							}
    
							string user, group;
							string combine = ((S_ISDIR(fileinfo.st_mode)) ? "d" : (S_ISLNK(fileinfo.st_mode)) ? "l" : "-");
							combine.append(((fileinfo.st_mode & S_IRUSR) ? "r" : "-"));
							combine.append(((fileinfo.st_mode & S_IWUSR) ? "w" : "-"));
							combine.append(((fileinfo.st_mode & S_IXUSR) ? "x" : "-")); 
							combine.append(((fileinfo.st_mode & S_IRGRP) ? "r" : "-"));
							combine.append(((fileinfo.st_mode & S_IWGRP) ? "w" : "-"));
							combine.append(((fileinfo.st_mode & S_IXGRP) ? "x" : "-"));
							combine.append(((fileinfo.st_mode & S_IROTH) ? "r" : "-"));
							combine.append(((fileinfo.st_mode & S_IWOTH) ? "w" : "-"));
							combine.append(((fileinfo.st_mode & S_IXOTH) ? "x" : "-"));

							if(pwd = getpwuid(fileinfo.st_uid))
								user = pwd->pw_name;
							else{
								user = "unknown";
								perror("error obtaining user name");
							}
	
							if(grp = getgrgid(fileinfo.st_gid))
								group = grp->gr_name;
							else{
								group = "unknown";
								perror("error obtaining group name");
							}
							
							results.push_back(combine);             //0
							results.push_back(std::to_string(fileinfo.st_nlink));    //1
							results.push_back(user);                //2
							results.push_back(group);               //3
							results.push_back(std::to_string(fileinfo.st_size));    //4
							results.push_back(timebuf);             //5
							results.push_back(file->d_name);                   //6

						}
					}
				}
				
				if(errno){//read issue
					perror("read error");
					exitVal = 1;
				}

				if((closedir(pDir))){//close error
					s2 = "cannot close ";
					s2 += s;
					perror(s2.c_str());
					exitVal = 1;
				}
			}
			if(!(flag & l_flag))
				sort(results.begin(),results.end(),comparison);
			printVector(results);
			results.clear();
		}
	}

	if(subDir.size() > 1)
	{
		sort(subDir.begin(),subDir.end(),comparison);
		char buf[PATH_MAX];
		if(!getcwd(buf,PATH_MAX))
			perror("can't read current working directory");
		static int level = 0;
		for(int i(0) ; i < subDir.size(); i++){//recursive function calls
			chdir(subDir[i].c_str());
			cout << setw(level++ * 4) << ' ' << setw(0) << subDir[i] << ':' << endl;
			theFunction(results,".");//subDir[i]);
			chdir(buf);
			level--;
		}
	}
}

void printVector(const vector<string>& results)//set spacing for printing environment
{
	int col = winSize();
	if(flag & l_flag)// l flag type printing
	{
		int sizes[7] = {0,0,0,0,0,0,0};

		for(int i(0); i < results.size();i++) // determine required width for columns
			for(int j(i) ; j < results.size(); j+=7)
				if(results[j].size()+1 > sizes[i%7])
					sizes[i%7] = results[j].size()+1;

		for(int i(0) ; i < results.size();i++)
		{
			if(!(i%7))
				cout << endl;
			cout<<setw(sizes[i%7]) << left << results[i];
			cout.flush();
		}
	}
	else // multiple files printing (no l flag)
	{
		int columns = col, iterator = 0, array[col];
		for(int i(0) ; i < col; i++)
			array[i] = 1;
		while(iterator < results.size())
		{
			int sum = 0;
			for(int i(0) ; i < columns ; i++)
				sum += array[i];
			
			if(sum > col && columns > 1){
				columns--;
				iterator = 0;
				for(int k(0); k < columns ; k++)
					array[k] = 1;
			}

			if(results[iterator].size()+2 >  array[iterator%columns])
				array[iterator%columns] = results[iterator].size()+2;
			iterator++;
		}
		for(int i(0); i < results.size(); i++){
			cout << setw(array[i%columns]) << left << results[i];
			if(i%columns == columns-1) cout << endl;
		}
	}
	
	if(results.size()) cout << endl;
}

bool comparison(string first, string second)//comparison to sort case insensitive and ignore the hidden abbreviation
{
	for(int i=0;i<first.size();i++)
		first[i] = tolower(first[i]);

	for(int i=0;i<second.size();i++)
		second[i] = tolower(second[i]);
	
	if(first.size() != 0 && first[0] == '.')
		first.erase(0,1);

	if(second.size() != 0 && second[0] == '.')
		second.erase(0,1);

	bool comp = first < second;
	return comp;
}
