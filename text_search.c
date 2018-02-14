/******************************************************************
 * @authors David Whitters
 * @date 2/13/18
 *
 * CIS 452
 * Dr. Dulimarta
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>

/** The maximum length of the search term. */
#define MAX_NUM_INPUT_CHARS 100u

int main(void)
{
    /* Holds user input. */
    char input[MAX_NUM_INPUT_CHARS] = {0};

    /* Process the input if the command isn't "quit". */
    while(1u)
    {
        /* Prompt user for the text to search for. */
        printf("Enter Command: ");
        /* Read the user input. */
        fgets(input, MAX_NUM_INPUT_CHARS, stdin);
        /* Remove newline character. */
        input[strcspn(input, "\n")] = 0;

        /* Make sure the user input a string. */
        if(strlen(input) > 0u)
        {
            /* Process command if the input isn't "quit". */
            if(strcmp(input, "quit") != 0u)
            {
                /* Holds ID of the parent process. */
                pid_t pid;
                int status;

                /* Fork. Exit on failure. */
                if((pid = fork()) < 0)
                {
                    perror("FORK FAILURE");
                    exit(EXIT_FAILURE);
                }
                else if(pid == 0)
                {
                    /* Child process execution. */
                    exit(EXIT_SUCCESS);
                }
                else
                {
                    /* Parent process execution. */
                    (void)wait(&status); /* Wait for child process to exit. */
                }
            }
            else
            {
                break;
            }
        }
    }

    return 0;
}
