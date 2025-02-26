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

void execute_command(char **cmd) {
    if (cmd == NULL || cmd[0] == NULL) {
        return; // No command entered, do nothing
    }

    pid_t pid = fork();
    if (pid == 0) {
        /* Child Process */
        pid_t child = getpid();
        setpgid(child, child);  // Put child in its own process group
        tcsetpgrp(STDIN_FILENO, child);  // Set terminal to child

        signal(SIGINT, SIG_DFL);  // Reset signals
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        execvp(cmd[0], cmd);  // Execute command
        perror("execvp failed"); // Only runs if execvp fails
        exit(EXIT_FAILURE);
    } 
    else if (pid < 0) {
        perror("fork failed");
        return;
    }

    /* Parent Process */
    setpgid(pid, pid);  // Put child process in its own group
    tcsetpgrp(STDIN_FILENO, pid);  // Give control of terminal to child

    int status;
    if (waitpid(pid, &status, 0) == -1) {  // Wait for the child to finish
        perror("waitpid failed");
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());  // Restore control of terminal to shell
}


// void execute_command(char **cmd) {
//     if (cmd == NULL || cmd[0] == NULL) {
//         return; // No command entered
//     }
//     }

//     pid_t pid = fork();
//     if (pid == 0) {
//         /* Child Process */
//         pid_t child = getpid();
//         setpgid(child, child);  // Put child in its own process group
//         tcsetpgrp(STDIN_FILENO, child);  // Set terminal to child

//         signal(SIGINT, SIG_DFL);  // Reset signals
//         signal(SIGQUIT, SIG_DFL);
//         signal(SIGTSTP, SIG_DFL);
//         signal(SIGTTIN, SIG_DFL);
//         signal(SIGTTOU, SIG_DFL);

//         execvp(cmd[0], cmd);  // Execute command
//         perror("execvp failed"); // Only runs if execvp fails
//         exit(EXIT_FAILURE);
//     } 
//     else if (pid < 0) {
//         perror("fork failed");
//         return;
//     }

//     /* Parent Process */
//     setpgid(pid, pid);  // Put child process in its own group
//     tcsetpgrp(STDIN_FILENO, pid);  // Give control of terminal to child

//     int status;
//     if (waitpid(pid, &status, 0) == -1) {  // Wait for the child to finish
//         perror("waitpid failed");
//     }

//     tcsetpgrp(STDIN_FILENO, getpgrp());  // Restore control of terminal to shell
// }


// void add_history(const char *line) {
//     if (line && *line) {
//         add_history(line);
//     }
//     // free(line);
//     line = NULL;
// }

// char *readline(const char *prompt) {
//     printf("%s", prompt);
//     size_t bufsize = 1024;
//     char *buffer = (char *)malloc(bufsize * sizeof(char));
//     if (!buffer) {
//         perror("Unable to allocate buffer");
//         exit(EXIT_FAILURE);
//     }

//     if (fgets(buffer, bufsize, stdin) == NULL) {
//         free(buffer);
//         return NULL;
//     }

//     size_t len = strlen(buffer);
//     if (len > 0 && buffer[len - 1] == '\n') {
//         buffer[len - 1] = '\0';
//     }

//     return buffer;
// }


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

    // If line is empty, return NULL
    if (line == NULL || *line == '\0') {
        return NULL;
    }

    // Trim whitespace before processing
    char *trimmed_line = trim_white(strdup(line));
    if (trimmed_line == NULL || *trimmed_line == '\0') {
        free(trimmed_line);
        return NULL;  // Return NULL if the line is just spaces
    }

    char **argv = malloc((arg_max / 2) * sizeof(char *));
    if (argv == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    char *line_copy = strdup(trimmed_line);
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
    free(line_copy);  // Free the duplicate line once done
    return argv;
}

// char **cmd_parse(char const *line) {
//     long arg_max = sysconf(_SC_ARG_MAX);
//     if (arg_max == -1) {
//         perror("sysconf");
//         exit(EXIT_FAILURE);
//     }

//     // If line is empty, return NULL
//     if (line == NULL || *line == '\0') {
//         return NULL;
//     }

//     char **argv = malloc((arg_max / 2) * sizeof(char *));
//     if (argv == NULL) {
//         perror("malloc");
//         exit(EXIT_FAILURE);
//     }

//     char *line_copy = strdup(line);
//     if (line_copy == NULL) {
//         perror("strdup");
//         exit(EXIT_FAILURE);
//     }

//     int argc = 0;
//     char *token = strtok(line_copy, " ");
//     while (token != NULL && argc < (arg_max / 2 - 1)) {
//         argv[argc] = strdup(token);
//         if (argv[argc] == NULL) {
//             perror("strdup");
//             exit(EXIT_FAILURE);
//         }
//         argc++;
//         token = strtok(NULL, " ");
//     }

//     argv[argc] = NULL;
//     free(line_copy);  // Free the duplicate line once done
//     return argv;
// }


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


// bool do_builtin(struct shell *sh, char **argv) {
//     if (strcmp(argv[0], "exit") == 0) {
//         sh_destroy(sh);
//         exit(EXIT_SUCCESS);
//     } else if (strcmp(argv[0], "cd") == 0) {
//         if (argv[1] == NULL) {
//             char *home = getenv("HOME");
//             if (home == NULL) {
//                 fprintf(stderr, "cd: HOME not set\n");
//                 return true;
//             }
//             if (chdir(home) != 0) {
//                 perror("cd");
//             }
//         } else {
//             if (chdir(argv[1]) != 0) {
//                 perror("cd");
//             }
//         }
//         return true;
//     } else if (strcmp(argv[0], "jobs") == 0) {
//         // Placeholder for jobs command
//         // In a real implementation, you would list background jobs here
//         printf("jobs: not implemented\n");
//         return true;
//     }
//     return false;
// }

// void execute_command(char **argv) {
//     if (argv[0] == NULL) {
//         return; // No command entered
//     }

//     pid_t pid = fork();
    
//     if (pid < 0) {
//         perror("fork");
//         exit(EXIT_FAILURE);
//     }
//     else if (pid == 0) {
//         // In child process
//         execvp(argv[0], argv);
//         perror("execvp"); // Only runs if execvp fails
//         exit(EXIT_FAILURE);
//     } 
//     else {
//         // In parent process, wait for child to finish
//         int status;
//         waitpid(pid, &status, 0);
//     }
// }

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
