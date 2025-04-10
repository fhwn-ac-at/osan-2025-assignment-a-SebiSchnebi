#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h> //exec ... commands
#include <sys/wait.h>
#include <time.h>

typedef struct command_line_arguments {
	int i;
	char const *s;
	bool b;
} cli_args;


cli_args parse_command_line (int const argc, char *argv[])
{
	cli_args args = {0, NULL, false};
	
	int optgot = -1;
	
	do
	{
		optgot = getopt(argc, argv, "i:s:b");
		printf("gotopt: %c --> %s\n", optgot, optarg);
		switch (optgot)
		{
			case 'i':
			args.i = atoi(optarg);
			break;
			case 's':
			args.s = optarg;
			break;
			case 'b':
			args.b = true;
			break;
			case '?':
			printf("Usage: %s -i <number> -s <string>, -b\n", argv[0]);
			exit(EXIT_FAILURE);
		}	
		
		
	} while (optgot != -1);
	
	if (args.i <= 0 || strlen(args.s) < 5)
	{
		printf("Usage: %s -i <number> -s <string>, -b\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	return args;
}


int child_labour()
{
	//printf("I am %d, child of %d\n", getpid(), getppid());
	printf("[%d] Doing some work...\n", getpid());
	sleep(5);
	printf("[%d] Job is done!\n", getpid());
	printf("[%d] Bring coal back to...[%d] \n", getpid(), getppid());
	
	return EXIT_SUCCESS;
}


int main(int argc, char *argv[], char *envp[]) 
{

	//cli_args const args = parse_command_line(argc, argv);
	//printf("\ni: %d, s: %s, b: %d\n", args.i, args.s, args.b);
	
	//start a process
	//execlp("ls", "-ls", "-l", NULL);
	//getpid == anzeigen von Prozess ID
	//forked == ID von Prozess der von fork erstellt wurde
	
	
	printf("My ProcessID is %d\n", getpid());
	
	printf("[%d] Sending child to mines...\n", getpid());
	
	
	pid_t forked = fork();
	
	if (forked == 0)
	{
		return child_labour();
	}

	printf("[%d] Enjoying some brady....\n", getpid());
	printf("[%d] Where is my Coal!!!1111!!!!!1\n", getpid());
	
	
	for(int i = 0; i < 5; i++)
	{
		int wstatus = 0;
		pid_t const waited = wait(&wstatus);
		
		if (WIFEXITED(wstatus))
		{
			printf("[%d] Child %d exited normally with return code %d\n", getpid(), waited, WEXITSTATUS(wstatus));
		}
	
		else if(WIFSIGNALED(wstatus))
			printf("[%d] Child %d terminated with %d\n", getpid(), waited, WTERMSIG(wstatus));
	
		else 
			printf("[%d] Child %d returned abnormally\n", getpid(), waited);
	
		printf("[%d] child returned %d, status is %d\n", getpid(), waited, wstatus);		
		
	}
	
	
//	printf("My ProcessID is %d| fork return value: %d\n", getpid(), forked);
	
	
    return 0;
}