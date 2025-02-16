#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include "lab.h"
#include <pwd.h>

void add_history(const char *line) {
    UNUSED(line); // Suppress unused parameter warning
    // This function is a placeholder for adding command history functionality.
    // In a real implementation, you might use a library like GNU Readline to handle history.
    // For now, this function does nothing.
}



char *readline(const char *prompt) {
    printf("%s", prompt);
    size_t bufsize = 1024;
    char *buffer = (char *)malloc(bufsize * sizeof(char));
    if (!buffer) {
        perror("Unable to allocate buffer");
        exit(EXIT_FAILURE);
    }

    if (fgets(buffer, bufsize, stdin) == NULL) {
        free(buffer);
        return NULL;
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    return buffer;
}

char *get_prompt(const char *env) {
    const char *prompt = getenv(env);
    if (prompt == NULL) {
        prompt = "shell>";
    }
    char *result = (char *)malloc(strlen(prompt) + 1);
    if (result == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strcpy(result, prompt);
    return result;
}

char **cmd_parse(char const *line) {
    long arg_max = sysconf(_SC_ARG_MAX);
    if (arg_max == -1) {
        perror("sysconf");
        exit(EXIT_FAILURE);
    }

    char **argv = malloc((arg_max / 2) * sizeof(char *));
    if (argv == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    char *line_copy = strdup(line);
    if (line_copy == NULL) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    int argc = 0;
    char *token = strtok(line_copy, " ");
    while (token != NULL && argc < (arg_max / 2 - 1)) {
        argv[argc] = strdup(token);
        if (argv[argc] == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
        argc++;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;

    free(line_copy);
    return argv;
}

void cmd_free(char **line) {
    if (line == NULL) {
        return;
    }

    for (int i = 0; line[i] != NULL; i++) {
        free(line[i]);
    }
    free(line);
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
        if (argv[1] == NULL) {
            char *home = getenv("HOME");
            if (home == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
                return true;
            }
            if (chdir(home) != 0) {
                perror("cd");
            }
        } else {
            if (chdir(argv[1]) != 0) {
                perror("cd");
            }
        }
        return true;
    } else if (strcmp(argv[0], "jobs") == 0) {
        // Placeholder for jobs command
        // In a real implementation, you would list background jobs here
        printf("jobs: not implemented\n");
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

    sh->prompt = get_prompt("SHELL_PROMPT");
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
