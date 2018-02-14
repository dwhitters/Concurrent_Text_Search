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
#include <sys/wait.h>

/** The maximum length of the search term. */
#define MAX_NUM_INPUT_CHARS 100u
/** The maximum number of characters expected in a file line. */
#define MAX_NUM_LINE_CHARS 512u

/**
    Searches a file for instances of text.

    @param file_path
        The path to the file to search.
    @param text
        Text to search for in the file.

    @return
        The number of instances of the text found in the file.
*/
int SearchFile(char * file_path, char * text)
{
    int num_text_instances = 0;
    FILE *fp;
    char file_line[MAX_NUM_LINE_CHARS] = {0};

    if((fp = fopen(file_path, "r")) == NULL)
    {
        perror("File open");
        exit(EXIT_FAILURE);
    }

    /* Read in lines from the file until the end of file is reached. */
    while(fgets(file_line, MAX_NUM_LINE_CHARS, fp) != NULL)
    {
        char * sub_str;
        /* Look for the input string in the line. */
        while((sub_str = strstr(file_line, text)) != NULL)
        {
            int index = sub_str - file_line; /* Get the index of the found string. */

            /*
                Check if the found substring was a word.
                To be a word, it has to start at index 0 or with a space in front of it.
                It must also end with either a space or a newline character.
            */
            if(((index == 0) ||
               (file_line[index - 1] == ' ')) &&
               ((file_line[index + strlen(text)] == ' ') ||
               (file_line[index + strlen(text)] == '\n')))
            {
                ++num_text_instances; /* Increment the text found count. */
            }
            /*
                Over-write the line from the file with the returned sub-string
                from strstr but offset by one. This ensures that the matched
                sub-string won't be matched again.
            */
            strcpy(file_line, sub_str + 1);
        }
    }

    /* Close the file. */
    if(fclose(fp) != 0)
    {
        perror("File close");
        exit(EXIT_FAILURE);
    }

    return num_text_instances;
}

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
                    printf("Num instances: %d\n", SearchFile("./books/The_Joy_Of_Life.txt", input));

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
