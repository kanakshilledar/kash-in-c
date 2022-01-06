#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// forward declarations for builtin functions
int cd_cmd(char **args);
int help_cmd(char **args);
int history_cmd(char **args);
int exit_cmd(char **args);

// list of builtin commands
char *builtin_str[] = {
        "cd",
        "help",
        "history",
        "exit",
};

// list of builtin command functions
int (*builtin_func[]) (char **) = {
        &cd_cmd,
        &help_cmd,
        &history_cmd,
        &exit_cmd,
};

// provides the number of builtin functions
int num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

// builtin functions implementations

// cd command
// changes directory args[0] is "cd". args[1] is directory.
// return 1 for continuous execution
int cd_cmd(char **args) {
    if (args[1] == NULL)
        fprintf(stderr, "kash: expected argument to \"cd\"\n");
    else {
        if (chdir(args[1]) != 0)
            perror("kash");
    }
    return 1;
}

// help command
// prints help message no arguments required
// return 1 for continuous execution
int help_cmd(char **args) {
    int i;
    printf("This is my personal implementation of a simple shell KASH\n");
    printf("Type program names and arguments, and hit enter. \n");
    printf("The following are built in:\n");

    for (i = 0; i < num_builtins(); i++)
        printf(" %s\n", builtin_str[i]);

    printf("Use man for manual of a specific command.\n");
    return 1;
}

// history command
// reads from the .history file and prints all the executed commands till now
// requires no arguments returns 1 for continuous execution
int history_cmd(char **args) {
        FILE *history_file = fopen(".history", "r");
        char c;
        while (1) {
            c = fgetc(history_file);
            if (c != EOF) {
                printf("%c", c);
            } else
                break;
        }
        fclose(history_file);
        return 1;
}

// exit command
// required to exit out of the shell
// returns 0 for ending the execution
int exit_cmd(char **args) {
    return 0;
}

// this function is used to launch a program and wait for its termination
// null terminated list of arguments
// returns 1 for continuous execution
int launcher(char **args) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("kash");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("kash");
    } else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}


// execute shell built-in or launch program
// null terminated list of arguments
// returns 1 or 0 according to command
int executer(char **args) {
    int i;
    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0)
            return (*builtin_func[i])(args);
    }

    return launcher(args);
}

// read input line from stdin
// return the same line
char *line_reader(void) {
#ifdef KASH_USE_STD_GETLINE
    char *lne = NULL;
    ssize_t bufsize = 0;
    if (getline(&line, &bufsize, stdin) == -1) {
         if (feof(stdin)) {
             exit(EXIT_SUCCESS);
         } else {
             perror("kash: getline\n");
             exit(EXIT_FAILURE);
         }
    }
    return line;

#else
#define KASH_RL_BUFSIZE 1024
    int bufsize = KASH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "kash: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // read character
        c = getchar();

        if (c == EOF) {
            exit(EXIT_SUCCESS);
        } else if (c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        // if buffer is exceeded then reallocate
        if (position >= bufsize) {
            bufsize += KASH_RL_BUFSIZE;
            buffer   = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "kash: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
#endif
}

#define KASH_TOK_BUFSIZE 64
#define KASH_TOK_DELIM " \t\r\n\a"

// get tokens form the line
// line as the argument
// null terminated array of tokens
char **split_line(char *line) {
    int bufsize = KASH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;

    if (!tokens) {
        fprintf(stderr, "kash: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, KASH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += KASH_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                free(tokens_backup);
                fprintf(stderr, "kash: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, KASH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

// loop for getting input and executing it
int main_loop() {
    char *line;
    char **args;
    int status;
    do {
        printf("\033[0;32m kash$ \033[0m");
        line = line_reader();
        args = split_line(line);
        status = executer(args);
        // writing the commands to the .history file
        FILE *history_file = fopen(".history", "a+");
        if (history_file == NULL) {
            printf("Error");
            exit(1);
        }
        fputs(line, history_file);
        // fputs doesnt have new line by default so a new line character is required
        fputs("\n", history_file);
        fclose(history_file);

        free(line);
        free(args);
    } while (status);
}

// main function
int main(int argc, char **argv) {
    main_loop();
    return EXIT_SUCCESS;
}
