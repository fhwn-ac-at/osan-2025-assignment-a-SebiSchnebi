#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <fcntl.h>
#include <string.h>

// Define the names of the two message queues
#define TASK_QUEUE   "/mq_tasks"
#define RESULT_QUEUE "/mq_results"

// Struct to hold command-line arguments
typedef struct {
    int workers;     // Number of worker processes
    int tasks;       // Total number of tasks to be distributed
    int queue_size;  // Maximum size of the message queue
} cli_args;

// Struct for a task message (sent to workers)
typedef struct {
    int effort;  // Simulated effort (in seconds)
} task_msg;

// Struct for result message (sent back from workers)
typedef struct {
    int worker_id;     // ID of the worker
    pid_t pid;         // PID of the worker
    int task_count;    // Number of tasks the worker processed
    int total_sleep;   // Total effort/sleep time
} result_msg;

// Parses command-line arguments using getopt
cli_args parse_command_line(int argc, char *argv[]) 
{
    cli_args args = {0, 0, 0}; // Initialize struct with zeros
    int opt;

    // Parse flags: -w (workers), -t (tasks), -s (queue size)
    while ((opt = getopt(argc, argv, "w:t:s:")) != -1) 
	{
        switch (opt) 
		{
            case 'w': args.workers = atoi(optarg); break;
            case 't': args.tasks = atoi(optarg); break;
            case 's': args.queue_size = atoi(optarg); break;
            default:
                // Invalid or missing argument
                fprintf(stderr, "Usage: %s -w <workers> -t <tasks> -s <queue_size>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Sanity check for arguments
    if (args.workers <= 0 || args.tasks <= 0 || args.queue_size <= 0) 
	{
        fprintf(stderr, "Invalid arguments.\n");
        exit(EXIT_FAILURE);
    }

    return args;
}

// Worker process function
void worker(int worker_id) 
{
    // Open message queues
    mqd_t mq_task = mq_open(TASK_QUEUE, O_RDONLY);
    mqd_t mq_result = mq_open(RESULT_QUEUE, O_WRONLY);

    if (mq_task == -1 || mq_result == -1) 
	{
        perror("Worker: mq_open");
        exit(EXIT_FAILURE);
    }

    int task_count = 0;
    int total_sleep = 0;

    // Continuously receive tasks
    while (1) 
	{
        task_msg task;
        // Receive a task message from the queue
        if (mq_receive(mq_task, (char *)&task, sizeof(task), NULL) == -1) 
		{
            perror("Worker: mq_receive");
            exit(EXIT_FAILURE);
        }

        // 0 effort = termination signal
        if (task.effort == 0) break;

        printf("Worker #%02d | Received task with effort %d\n", worker_id, task.effort);
        sleep(task.effort); // Simulate work
        task_count++;
        total_sleep += task.effort;
    }

    // Send result message back to ventilator
    result_msg result = { worker_id, getpid(), task_count, total_sleep };
    mq_send(mq_result, (const char *)&result, sizeof(result), 0);

    // Close message queues and exit
    mq_close(mq_task);
    mq_close(mq_result);
    exit(0);
}

// Main program
int main(int argc, char *argv[]) 
{
    cli_args args = parse_command_line(argc, argv); // Get CLI arguments
    srand(time(NULL)); // Seed for random effort

    // Log startup message
    printf("%02d:%02d:%02d | Ventilator | Starting %d workers for %d tasks and a queue size of %d\n",
           localtime(&(time_t){time(NULL)})->tm_hour,
           localtime(&(time_t){time(NULL)})->tm_min,
           localtime(&(time_t){time(NULL)})->tm_sec,
           args.workers, args.tasks, args.queue_size);

    // Create task queue
    struct mq_attr attr = { .mq_maxmsg = args.queue_size, .mq_msgsize = sizeof(task_msg) };
    mqd_t mq_task = mq_open(TASK_QUEUE, O_CREAT | O_WRONLY, 0600, &attr);

    // Create result queue
    mqd_t mq_result = mq_open(RESULT_QUEUE, O_CREAT | O_RDONLY, 0600, &(struct mq_attr){ .mq_maxmsg = args.workers, .mq_msgsize = sizeof(result_msg) });

    if (mq_task == -1 || mq_result == -1) 
	{
        perror("Ventilator: mq_open");
        exit(EXIT_FAILURE);
    }

    // Fork worker processes
    for (int i = 1; i <= args.workers; i++) 
	{
        pid_t pid = fork();
        if (pid == 0) // Child process
		{
            printf("%02d:%02d:%02d | Worker #%02d | Started worker PID %d\n",
                   localtime(&(time_t){time(NULL)})->tm_hour,
                   localtime(&(time_t){time(NULL)})->tm_min,
                   localtime(&(time_t){time(NULL)})->tm_sec,
                   i, getpid());
            worker(i); // Enter worker loop
        }
    }

    // Distribute tasks
    printf("%02d:%02d:%02d | Ventilator | Distributing tasks\n",
           localtime(&(time_t){time(NULL)})->tm_hour,
           localtime(&(time_t){time(NULL)})->tm_min,
           localtime(&(time_t){time(NULL)})->tm_sec);

    for (int i = 0; i < args.tasks; i++) 
	{
        task_msg task = { rand() % 10 + 1 }; // Random effort 1-10
        printf("%02d:%02d:%02d | Ventilator | Queuing task #%d with effort %d\n",
               localtime(&(time_t){time(NULL)})->tm_hour,
               localtime(&(time_t){time(NULL)})->tm_min,
               localtime(&(time_t){time(NULL)})->tm_sec,
               i + 1, task.effort);
        mq_send(mq_task, (const char *)&task, sizeof(task), 0);
    }

    // Send termination tasks to workers
    printf("%02d:%02d:%02d | Ventilator | Sending termination tasks\n",
           localtime(&(time_t){time(NULL)})->tm_hour,
           localtime(&(time_t){time(NULL)})->tm_min,
           localtime(&(time_t){time(NULL)})->tm_sec);

    for (int i = 0; i < args.workers; i++) 
	{
        task_msg term = { 0 }; // 0 = termination
        mq_send(mq_task, (const char *)&term, sizeof(term), 0);
    }

    // Collect results from workers
    printf("%02d:%02d:%02d | Ventilator | Waiting for workers to terminate\n",
           localtime(&(time_t){time(NULL)})->tm_hour,
           localtime(&(time_t){time(NULL)})->tm_min,
           localtime(&(time_t){time(NULL)})->tm_sec);

    for (int i = 0; i < args.workers; i++) 
	{
        result_msg res;
        mq_receive(mq_result, (char *)&res, sizeof(res), NULL);
        printf("%02d:%02d:%02d | Ventilator | Worker %d processed %d tasks in %d seconds\n",
               localtime(&(time_t){time(NULL)})->tm_hour,
               localtime(&(time_t){time(NULL)})->tm_min,
               localtime(&(time_t){time(NULL)})->tm_sec,
               res.worker_id, res.task_count, res.total_sleep);
    }

    // Wait for all workers to exit
    for (int i = 0; i < args.workers; i++) 
	{
        int status;
        pid_t pid = wait(&status);
        if (WIFEXITED(status)) 
		{
            printf("%02d:%02d:%02d | Ventilator | Worker with PID %d exited with status %d\n",
                   localtime(&(time_t){time(NULL)})->tm_hour,
                   localtime(&(time_t){time(NULL)})->tm_min,
                   localtime(&(time_t){time(NULL)})->tm_sec,
                   pid, WEXITSTATUS(status));
        }
    }

    // Cleanup: close and unlink queues
    mq_close(mq_task);
    mq_close(mq_result);
    mq_unlink(TASK_QUEUE);
    mq_unlink(RESULT_QUEUE);

    return 0;
}
