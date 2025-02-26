#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include "lab.h"
#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <sys/types.h>
#include <sys/wait.h>


char *get_prompt(const char *env) {
    const char *prompt = getenv(env);
    if (prompt == NULL) {
        prompt = "shell>";
    }
    char *result = strdup(prompt);
    if (result == NULL) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    // strcpy(result, prompt);
    return result;
}

char **cmd_parse(char const *line) {
    long arg_max = sysconf(_SC_ARG_MAX);
    if (arg_max == -1) {
        perror("sysconf");
        exit(EXIT_FAILURE);
    }

    if (line == NULL || *line == '\0') {
        // Return an empty array if input is NULL or empty
        char **argv = malloc(sizeof(char *));
        if (argv == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        argv[0] = NULL;
        return argv;
    }

    // strdup first, then trim, so we free correctly
    char *line_copy = strdup(line);  // Duplicate the input line
    if (line_copy == NULL) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    char *trimmed_line = trim_white(line_copy);  // Trim leading and trailing spaces
    if (*trimmed_line == '\0') {  // If only whitespace remains
        free(line_copy);  // Free the duplicated line
        char **argv = malloc(sizeof(char *));  // Return an empty array
        if (argv == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        argv[0] = NULL;
        return argv;
    }

    char **argv = malloc((arg_max / 2) * sizeof(char *));  // Allocate memory for arguments
    if (argv == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    int argc = 0;
    char *token = strtok(trimmed_line, " ");  // Tokenize the trimmed line by spaces
    while (token != NULL && argc < (arg_max / 2 - 1)) {
        argv[argc] = strdup(token);  // Duplicate each token
        if (argv[argc] == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
        argc++;
        token = strtok(NULL, " ");
    }

    argv[argc] = NULL;  // Null-terminate the argument array
    free(line_copy);  // Free the original line_copy after processing
    return argv;
}


void cmd_free(char **line) {
    if (line == NULL) {
        return;
    }

    for (int i = 0; line[i] != NULL; i++) {
        free(line[i]);  // Free each individual argument
    }
    free(line);  // Free the array holding the arguments
}

char *trim_white(char *line) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*line)) line++;

    if (*line == 0)  // All spaces?
    return line;

    // Trim trailing space
    end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';

    return line;
}

bool do_builtin(struct shell *sh, char **argv) {
    if (strcmp(argv[0], "exit") == 0) {
        sh_destroy(sh);
        exit(EXIT_SUCCESS);
    } else if (strcmp(argv[0], "cd") == 0) {
        const char *path = argv[1];
        if (path == NULL) {
            path = getenv("HOME");
            if (path == NULL) {
                struct passwd *pw = getpwuid(getuid());
                if (pw == NULL) {
                    fprintf(stderr, "cd: Unable to determine home directory\n");
                    return true;
                }
                path = pw->pw_dir;
            }
        }
        if (chdir(path) != 0) {
            perror("cd");
        } else {
            printf("Changed directory to: %s\n", path);
        }
        return true;
    } else if (strcmp(argv[0], "history") == 0) {
        HIST_ENTRY **history = history_list();
        if (history) {
            for (int i = 0; history[i]; i++) {
                printf("%d: %s\n", i + 1, history[i]->line);
            }
        }
        return true;
    }
    return false;
}

void sh_init(struct shell *sh) {
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);

    if (sh->shell_is_interactive) {
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp())) {
            kill(-sh->shell_pgid, SIGTTIN);
        }

        sh->shell_pgid = getpid();
        if (setpgid(sh->shell_pgid, sh->shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(EXIT_FAILURE);
        }

        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
    }

    // Ignore signals in the shell
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    sh->prompt = get_prompt("MY_PROMPT");
}

void sh_destroy(struct shell *sh) {
    free(sh->prompt);
}

int change_dir(char **dir) {
    if (dir[1] == NULL) {
        const char *home = getenv("HOME");
        if (home == NULL) {
            struct passwd *pw = getpwuid(getuid());
            home = pw->pw_dir;
        }
        return chdir(home);
    } else {
        return chdir(dir[1]);
    }
}

void parse_args(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':
                printf("Shell version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
                exit(EXIT_SUCCESS);
            default:
                fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}
