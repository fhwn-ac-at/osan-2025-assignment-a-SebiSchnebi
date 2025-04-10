#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>

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



int main(int argc, char *argv[], char *envp[]) 
{

	cli_args const args = parse_command_line(argc, argv);
	printf("\ni: %d, s: %s, b: %d\n", args.i, args.s, args.b);
	
    return 0;
}