#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h> //exec ... commands
#include <sys/wait.h>
#include <time.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>


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

struct work_message {
	int work; 
	
};

int child_labour()
{
	
	mqd_t command_queue = mq_open("/mq_211063", O_RDONLY);
	printf("[%d] mq_open returned %d\n", getpid(), command_queue);
	
	
	if (command_queue == -1)
	{
		printf("Failed to open message queue\n");
		return EXIT_FAILURE;
	}
	
	printf("[%d] Waiting for instructions...\n", getpid());
	struct work_message instructions;



	
	
	int const received = mq_receive(command_queue, (void *)&instructions, sizeof(struct work_message), NULL);
	if (received == -1)
	{
		fprintf(stderr, "failed to receive instructions\n");
		return EXIT_FAILURE;
	}
	
	printf("[%d] Received message of size %d bytes: work to do %d\n", getpid(), received, instructions.work);
	
	//printf("I am %d, child of %d\n", getpid(), getppid());
	printf("[%d] Doing some work...\n", getpid());
	sleep(instructions.work);
	printf("[%d] Job is done!\n", getpid());
	printf("[%d] Bring coal back to...[%d] \n", getpid(), getppid());
	
	mq_close(command_queue);
	return getpid();
}




int main(int argc, char *argv[], char *envp[]) 
{
	
	struct mq_attr queue_options = {
		.mq_maxmsg = 1,
		.mq_msgsize = sizeof(struct work_message),
	};
	
	
	mqd_t command_queue = mq_open("/mq_211063", O_WRONLY | O_CREAT, S_IRWXU, &queue_options);
	printf("[%d] mq_open returned %d\n", getpid(), command_queue);
	if (command_queue == -1)
	{
		printf("Failed to open message queue\n");
		return EXIT_FAILURE;
	}
	//cli_args const args = parse_command_line(argc, argv);
	//printf("\ni: %d, s: %s, b: %d\n", args.i, args.s, args.b);
	
	//start a process
	//execlp("ls", "-ls", "-l", NULL);
	//getpid == anzeigen von Prozess ID
	//forked == ID von Prozess der von fork erstellt wurde
	
	
	printf("My ProcessID is %d\n", getpid());
	
	printf("[%d] Sending child to mines...\n", getpid());
	
	
	struct work_message instructions = {5};
	int send = mq_send(command_queue, (void *)&instructions, sizeof(struct work_message), 0);
	if (send == -1)
	{
		fprintf(stderr, "Failed to send instructions\n");
		return EXIT_FAILURE;
	}
	
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
	mq_close(command_queue);
	//mq_unlink("/mq_211063");
	
    return 0;
}