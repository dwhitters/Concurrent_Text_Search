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
#include <signal.h>

/** The maximum length of the search term. */
#define MAX_NUM_INPUT_CHARS 100u
/** The maximum number of characters expected in a file line. */
#define MAX_NUM_LINE_CHARS 512u
/** The pipe index for the read file descriptor. */
#define READ 0
/** The pipe index for the write file descriptor. */
#define WRITE 1
/** The maximum number of input files allowed. */
#define MAX_NUM_INPUT_FILES 10

/**
    Signal handler. The child processes are terminated within this
    upon receiving a SIGUSR1 signal.

    @param sig_num
        The signal to handle.
*/
void Sig_Handler(int sig_num)
{
    switch(sig_num)
    {
        case SIGUSR1:
            /* Print out the process ids of the parent and child. */
            printf("The parent of child %d is %d\n", getpid(), getppid());
            exit(EXIT_SUCCESS);
            break;
    }
}

/**
    Gathers file_paths from a text file.

    @param file_path
        The path to the file to search.
    @param files
        The file paths that were gathered.
    @param num_files
        Pointer to the integer variable that contains how many files there are to process.
*/
void GetFilePathsFromTextFile(char * file_path, char files[MAX_NUM_INPUT_FILES][MAX_NUM_LINE_CHARS], int * num_files)
{
    FILE *fp;

    if((fp = fopen(file_path, "r")) == NULL)
    {
        perror("Opening initial text file");
        exit(EXIT_FAILURE);
    }

    /* Read in lines from the file until the end of file is reached. */
    for(int i = 0; i < (MAX_NUM_INPUT_FILES - 1); ++i)
    {
        if(fgets(files[i], MAX_NUM_LINE_CHARS, fp) != NULL)
        {
            /* Remove newline character. */
            files[i][strcspn(files[i], "\n")] = 0;
            printf("File%d: %s\n", i, files[i]);
            *num_files += 1;
        }
        else
        {
            break;
        }
    }

    /* Close the file. */
    if(fclose(fp) != 0)
    {
        perror("File close");
        exit(EXIT_FAILURE);
    }
}
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
        printf("Error opening: %s\n", file_path);
        perror("Opening file");
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

int main(int argc, char * argv[])
{
    /* Holds user input. */
    char input[MAX_NUM_INPUT_CHARS] = {0};
    /* The number of files to search. */
    int num_files = 0;
    /* Holds ID of the parent process. */
    pid_t pid;
    /* Contains all files paths to be searched. */
    char files[MAX_NUM_INPUT_FILES][MAX_NUM_LINE_CHARS] = {0};
    /* Contains all child PIDs. */
    pid_t child_pids[MAX_NUM_INPUT_FILES] = {0};

    if(argc > 1)
    {
        /* Only set the number of files if it's within limits. */
        num_files = ((argc -1) > MAX_NUM_INPUT_FILES) ? 0 : (argc - 1);
        if(num_files == 0)
        {
            printf("Too many files!\n");
        }
    }
    else
    {
        char text_file_path[MAX_NUM_INPUT_CHARS]; /* The text file path. */
        printf("Please enter the text file that contains the paths to the files to search: ");
        fgets(text_file_path, MAX_NUM_INPUT_CHARS, stdin);
        /* Remove newline character. */
        text_file_path[strcspn(text_file_path, "\n")] = 0;
        GetFilePathsFromTextFile(text_file_path, files, &num_files);
    }

    /* The pipe from parent to child. */
    int downstream_pipes[MAX_NUM_INPUT_FILES][2] = {0};
    /* The pipe from child to parent. */
    int upstream_pipes[MAX_NUM_INPUT_FILES][2] = {0};

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

        if(argc > 1)
        {
            strcpy(files[i], argv[i + 1]);
            printf("File%d: %s\n", i, files[i]);
        }
    }

    /* Setup the child processes. */
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

            /* Setup the SIGUSR1 signal for the parent process to use to kill the children. */
            if(signal(SIGUSR1, Sig_Handler) == SIG_ERR)
            {
                perror("Signal setup error.");
                exit(EXIT_FAILURE);
            }

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

            while(1)
            {
                if(read(downstream_pipes[i][READ], search_text, MAX_NUM_INPUT_CHARS) == -1)
                {
                    perror("Read failure.");
                    exit(EXIT_FAILURE);
                }

                /* Convert the search num value to a string to send it to the parent over the pipe. */
                char val[100] = {0};
                sprintf(val, "%d", SearchFile(file_path, search_text));
                write(upstream_pipes[i][WRITE], val, strlen(val) + 1); /* Send the value to the parent. */
            }
        }
        else
        {
            /* Parent process execution. */
            printf("The parent PID is %d\n", getpid());
            /* Store the child PID that is associated with the current book. */
            child_pids[i] = pid;
            /* Send the file path to the children via pipe. Include the NULL character. */
            if(write(downstream_pipes[i][WRITE], files[i], strlen(files[i]) + 1) == -1)
            {
                perror("Write failure.");
                exit(EXIT_FAILURE);
            }
        }
    }

    /* Start the program if the number of files is greater than zero. */
    while(num_files > 0)
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
                for(int i = 0; i < num_files; ++i)
                {
                    /* Send the text to search to the children via pipe. Include the NULL character. */
                    if(write(downstream_pipes[i][WRITE], input, strlen(input) + 1) == -1)
                    {
                        perror("Write failure.");
                        exit(EXIT_FAILURE);
                    }
                }
                char val[100] = {0}; /* Contains the stringified number of search text instances found. */
                for(int i = 0; i < num_files; ++i)
                {
                    /* Get the number of search text instances found. */
                    read(upstream_pipes[i][READ], val, 100);
                    printf("File: %s\n\t%s: %s\n", files[i], input, val);
                }
            }
            else
            {
                for(int i = 0; i < num_files; ++i)
                {
                    /* Send the SIGUSR1 signal to all of the child processes. */
                    kill(child_pids[i], SIGUSR1);
                }
                num_files = 0; /* Break out of the while loop. */
            }
        }
    }

    pid_t wpid; /* The returned pid from the wait command. */
    int status;
    while((wpid = wait(&status)) > 0); /* Wait for all child processes to exit. */

    return 0;
}
