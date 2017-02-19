#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
using namespace std;
char current[200];
bool background = false;
int in,out,argnum,pipes,back = 0;
char *arguments[100],*input, *output;
void executecommand(char *args[])
{
	int p;
	p = 0;
	while(args[p] != NULL)
	{
		if(strcmp(args[p],"<") == 0)	//Checking read operator
		{
			args[p] = NULL;
			in = 1;
			input = args[p + 1];
		}
		else if(strcmp(args[p],">") == 0)	//Checking write operator
		{
			args[p] = NULL;
			out = 1;
			output = args[p + 1];
		}
		else if(strcmp(args[p],">>") == 0)	//Checking append operator
		{
			args[p] = NULL;
			out = 2;
			output = args[p + 1];
		}
		else if(strcmp(args[p],"&") == 0)	//Removing background symbol '&'
		{
			args[p] = NULL;
		}
		p++;
	}
	if(in == 1)	//Input mode
	{
		in = open(input, O_RDONLY);
		dup2(in, 0);
		close(in);
	}
	if(out == 1)	//Output write mode
	{
		out = open(output, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
		dup2(out, 1);
		close(out);
	}
	else if(out == 2)	//Output append mode
	{
		out = open(output, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR | S_IROTH | S_IWOTH);
		dup2(out, 1);
		close(out);
	}
	execvp(args[0],args);	//Command execution
	in = out = 0;
	input = output = NULL;
	exit(0);
}
void executepipes()
{
	pid_t pid;
	char *commands[pipes+1][100];	//Declaring 'pipes+1' commands
	int pipefd[2*pipes];
	for(int i=0;i<pipes;i++)	//Initiating pipe fd
		pipe(pipefd + (2*i));
	int j,k,l;
	j = k = l = 0;
	for(j=0;j<argnum;)			//Separating 'pipes+1' commands
	{
		if(strcmp(arguments[j],"|") == 0)
		{
			commands[k][l] = NULL;
			k++;
			l = 0;
			j++;
			continue;
		}
		commands[k][l] = arguments[j];
		j++;
		l++;
	}
	commands[k][l] = NULL;
	for(int i=0;i<=pipes;i++)		//Executing the commands
	{
		pid = fork();
		if(pid==0)
		{
			if(i != 0)
			{
				dup2(pipefd[2*i - 2],0);	//Input mode
			}
			if(i != pipes)
			{
				dup2(pipefd[2*i + 1],1);	//Output mode
			}
			for(int j=0;j<(2*pipes);j++)
				close(pipefd[j]);
			executecommand(commands[i]);	//Call to execute command
		}
	}
	for(int z=0;z<(2*pipes);z++)	// Closing pipe file descriptors
		close(pipefd[z]);
	for(int z=0;z<=pipes;z++)		//Parent waiting for child processes to complete
	{
		if(z != pipes || !background)
			wait(NULL);
	}
	return;
}
void tokenize(char *cmd, char **args)
{
	while (*cmd != '\0')		//Tokenizing command cmd to args
	{
		while (*cmd == ' ' || *cmd == '\t' || *cmd == '\n')
			*cmd++ = '\0';
		*args++ = cmd;
		argnum++;
		while (*cmd != ' ' && *cmd != '\0' && *cmd != '\t' && *cmd != '\n')
			cmd++;
	}
	*args = NULL;
	for(int q=0;q<argnum;q++)
	{
		if(strcmp(arguments[q],"|") == 0)	//Counting number of pipes
			pipes+=1;
		if(strcmp(arguments[q],"&") == 0)	//Counting number of '&' for error check
			back+=1;
	}
}
int main()
{
	char command[200];
	pid_t pid=1;
	system("clear");
	string pwd;
	while(pid!=0)
	{
		background=false;
		in = 0;
		out = 0;
		input = NULL;
		output = NULL;
		pipes = 0;
		back = 0;
		while(argnum>=0)
		{
			arguments[argnum] = NULL;
			argnum--;
		}
		argnum = 0;
		pwd = getcwd(current, sizeof(current));	//Getting current directory
		cout<<"bash:"<<pwd<<"$ ";
		char ch=' ';
		int i;
		i = -1;
		while(ch!='\n')		//Input of command
		{
			ch = getchar();
			i++;
			if(ch == '\n')
				break;
			command[i] = ch;
		}
		command[i] = '\0';
		tokenize(command,arguments);	//Call to tokenize the command
		if(arguments[0] == NULL || strcmp(arguments[0],"") == 0)	//Check for empty command
			continue;
		if(strcmp(arguments[argnum-1],"") == 0)		//Removing extra "" string that may be in tokenized arguments
		{
			arguments[argnum-1] = NULL;
			argnum--;
		}
		if(back > 1)		//Reporting error for > 1 '&' operators
		{
			cout<<"bash: syntax error"<<endl;
			continue;
		}
		if(strcmp(arguments[argnum-1],"&") == 0) //Checking background process
		{
			arguments[argnum-1] = NULL;
			argnum--;
			background = true;
		}
		if(strcmp(arguments[0],"exit") == 0 && arguments[1] == NULL) //Checking for exit command
			exit(0);
		if(strcmp(arguments[0],"cd") == 0)		//Checking for change of directory
		{
			if(arguments[1] == NULL)
				continue;
			if(chdir(arguments[1]) !=0)
				perror(arguments[1]);
			continue;
		}
		if(pipes != 0)
		{
			executepipes();	//Call for handling pipes
			continue;
		}
		pid=fork();
		if(pid<0)
		{
			cout<<"fork failed\n";
			exit(-1);
		}
		else if(pid==0)
			executecommand(arguments);	//Call for executing normal command
		else
		{
			if(!background)		//Parent waiting for child
				wait(NULL);
		}
	}
	return 0;
}