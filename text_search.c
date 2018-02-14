/******************************************************************
 * @authors David Whitters
 * @date 2/15/18
 *
 * CIS 452
 * Dr. Dulimarta
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

/** The maximum length of the search term. */
#define MAX_NUM_INPUT_CHARS 100u
/** The maximum number of characters expected in a file line. */
#define MAX_NUM_LINE_CHARS 512u
/** The pipe index for the read file descriptor. */
#define READ 0
/** The pipe index for the write file descriptor. */
#define WRITE 1

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
    /* The pipe from parent to child. */
    int downstream_pipes[2][2];
    /* The pipe from child to parent. */
    int upstream_pipes[2][2];
    /* The number of files to search. */
    int num_files = 2;

    /* Create the pipes. */
    for(int i = 0; i < num_files; ++i)
    {
        if(pipe(downstream_pipes[i]) < 0)
        {
            perror("Downstream pipe init failure.");
            exit(EXIT_FAILURE);
        }
        if(pipe(upstream_pipes[i]) < 0)
        {
            perror("Upstream pipe init failure.");
            exit(EXIT_FAILURE);
        }
    }

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
            int num_alpha_chars = 0;
            /* Search for the text if the input is alphabetic. */
            for(num_alpha_chars = 0; num_alpha_chars < strlen(input); ++num_alpha_chars)
            {
                if(isalpha(input[num_alpha_chars]) == 0)
                {
                    break; /* Break if one of the characters in non alphabetic. */
                }
            }

            /* Search for the input if the input consists entirely of alphabetic characters. */
            if(num_alpha_chars == strlen(input))
            {
                /* Holds ID of the parent process. */
                pid_t pid;
                pid_t wpid; /* The returned pid from the wait command. */
                int status;

                char * books[2] = {"./books/The_Joy_Of_Life.txt", "./books/The_Sugar_Creek_Gang_Digs_For_Treasure.txt"};
                pid_t child_pids[2] = {0};
                for(int i = 0; i < num_files; ++i)
                {

                    /* Fork. Exit on failure. */
                    if((pid = fork()) < 0)
                    {
                        perror("FORK FAILURE");
                        exit(EXIT_FAILURE);
                    }
                    else if(pid == 0)
                    {
                        /* Child process execution. */
                        /* Close unused ends of the pipes by the children. */
                        close(downstream_pipes[i][WRITE]);
                        close(upstream_pipes[i][READ]);

                        /* Will contain the search text received from the pipe. */
                        char file_path[MAX_NUM_INPUT_CHARS];
                        char search_text[MAX_NUM_INPUT_CHARS];
                        if(read(downstream_pipes[i][READ], file_path, MAX_NUM_INPUT_CHARS) == -1)
                        {
                            perror("Read failure.");
                            exit(EXIT_FAILURE);
                        }
                        printf("File path: %s\n", file_path);
                        if(read(downstream_pipes[i][READ], search_text, MAX_NUM_INPUT_CHARS) == -1)
                        {
                            perror("Read failure.");
                            exit(EXIT_FAILURE);
                        }

                        /* Convert the search num value to a string to send it to the parent over the pipe. */
                        char val[100] = {0};
                        sprintf(val, "%d", SearchFile(file_path, search_text));
                        write(upstream_pipes[i][WRITE], val, strlen(val) + 1); /* Send the value to the parent. */
                        /* Print out the process ids of the parent and child to the log. */
                        printf("The parent of child %d is %d\n", getpid(), getppid());
                        exit(EXIT_SUCCESS);
                    }
                    else
                    {
                        /* Parent process execution. */
                        printf("The parent PID is %d\n", getpid());
                        /* Store the child PID that is associated with the current book. */
                        child_pids[i] = pid;
                        /* Send the file path to the children via pipe. Include the NULL character. */
                        if(write(downstream_pipes[i][WRITE], books[i], strlen(books[i]) + 1) == -1)
                        {
                            perror("Write failure.");
                            exit(EXIT_FAILURE);
                        }
                        sleep(1); /* Make sure the pipe is open before writing to it again. */
                        /* Send the text to search to the children via pipe. Include the NULL character. */
                        if(write(downstream_pipes[i][WRITE], input, strlen(input) + 1) == -1)
                        {
                            perror("Write failure.");
                            exit(EXIT_FAILURE);
                        }
                    }
                }

                char val[100] = {0}; /* Contains the stringified number of search text instances found. */
                while((wpid = wait(&status)) > 0) /* Wait for all child processes to exit. */
                {
                    for(int i = 0; i < num_files; ++i)
                    {
                        if(child_pids[i] == wpid)
                        {
                            /* Get the number of search text instances found. */
                            read(upstream_pipes[i][READ], val, 100);
                            printf("Book: %s\n\t%s: %s\n", books[i], input, val);
                        }
                    }
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
