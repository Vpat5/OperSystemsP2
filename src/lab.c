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


// char *get_prompt(const char *env) {
//     const char *prompt = getenv(env);
//     if (prompt == NULL) {
//         prompt = "shell>";
//     }
//     char *result = strdup(prompt);
//     if (result == NULL) {
//         perror("strdup");
//         exit(EXIT_FAILURE);
//     }
//     // strcpy(result, prompt);
//     return result;
// }

char *get_prompt(const char *env) {
    char *rval = NULL;
    const char *tmp = getenv(env);
    if (!tmp) {
        tmp = "shell>";
    }
    int n = strlen(tmp) + 1;
    rval = (char*)malloc(sizeof(char)*n);
    strncpy(rval, tmp, n);
    return rval;
}

// char **cmd_parse(char const *line) {
//     long arg_max = sysconf(_SC_ARG_MAX);
//     if (arg_max == -1) {
//         perror("sysconf");
//         exit(EXIT_FAILURE);
//     }

//     if (line == NULL || *line == '\0') {
//         // Return an empty array if input is NULL or empty
//         char **argv = malloc(sizeof(char *));
//         if (argv == NULL) {
//             perror("malloc");
//             exit(EXIT_FAILURE);
//         }
//         argv[0] = NULL;
//         return argv;
//     }

//     // strdup first, then trim, so we free correctly
//     char *line_copy = strdup(line);  // Duplicate the input line
//     if (line_copy == NULL) {
//         perror("strdup");
//         exit(EXIT_FAILURE);
//     }

//     char *trimmed_line = trim_white(line_copy);  // Trim leading and trailing spaces
//     if (*trimmed_line == '\0') {  // If only whitespace remains
//         free(line_copy);  // Free the duplicated line
//         char **argv = malloc(sizeof(char *));  // Return an empty array
//         if (argv == NULL) {
//             perror("malloc");
//             exit(EXIT_FAILURE);
//         }
//         argv[0] = NULL;
//         return argv;
//     }

//     char **argv = malloc((arg_max / 2) * sizeof(char *));  // Allocate memory for arguments
//     if (argv == NULL) {
//         perror("malloc");
//         exit(EXIT_FAILURE);
//     }

//     int argc = 0;
//     char *token = strtok(trimmed_line, " ");  // Tokenize the trimmed line by spaces
//     while (token != NULL && argc < (arg_max / 2 - 1)) {
//         argv[argc] = strdup(token);  // Duplicate each token
//         if (argv[argc] == NULL) {
//             perror("strdup");
//             exit(EXIT_FAILURE);
//         }
//         argc++;
//         token = strtok(NULL, " ");
//     }

//     argv[argc] = NULL;  // Null-terminate the argument array
//     free(line_copy);  // Free the original line_copy after processing
//     return argv;
// }

char **cmd_parse(char const *line) {
    const long arg_max = sysconf(_SC_ARG_MAX);
    char *tmp = strdup(line);
    char *save = NULL;
    if (!tmp) {
        fprintf(stderr, "strdup failed\n");
        abort();
    }
    char **rval = (char**)calloc(arg_max, sizeof(char*));
    char *tok = strtok_r(tmp, " ", &save);


    for (int i = 0; tok && i < arg_max; i++) {
        rval[i] = tok;
        tok = strtok_r(NULL, " ", &save);
    }
    return rval;
}

void cmd_free(char **line) {
    char *tmp = *line;
    free(tmp);
    free((void *)line);
}

char *trim_white(char *line) 
{
    if(!line) 
    {
        return line;
    }

    char *curr = line;
    while (curr && isspace(*curr)) 
        curr++;
    if (*curr == '\0') {
        *line = '\0';
        return line;
    }

    if (curr != line) 
    {
        int len = strlen(curr);
        memmove(line, curr, len + 1);
    }

    curr = line + strlen(line) - 1;
    while (curr > line && isspace(*curr)) {
        curr--;
    }
    *(curr + 1) = '\0';

    return line;
    
}

bool do_builtin(struct shell *sh, char **argv) {
   bool rval = false;
   if (strcmp(argv[0], "cd") == 0) 
   {
       if (change_dir(argv)) 
       {
           fprintf(stderr, "Failed to change directory\n");
       }
       rval = true;
   } 
   else if (strcmp(argv[0], "exit") == 0) 
   {
         sh_destroy(sh);
   }
    else if (strcmp(argv[0], "history") == 0) 
    {
        for (int i = history_base; i < history_length; i++){
            HIST_ENTRY *curr = history_get(i);
            printf("%d: %s\n", i, curr->line);
        }
        rval = true;
   }
   return rval;
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

// void sh_init(struct shell *sh)
// {
//     using_history();
//     signal;
// }

void sh_destroy(struct shell *sh) {
    if (sh->prompt) {
        free(sh->prompt);
    }
    clear_history();
    exit(EXIT_SUCCESS);
    
}

// int change_dir(char **dir) {
//     if (dir[1]) {
//         const char *home = getenv("HOME");
//         if (home == NULL) {
//             struct passwd *pw = getpwuid(getuid());
//             home = pw->pw_dir;
//         }
//         return chdir(home);
//     } else {
//         return chdir(dir[1]);
//     }
// }

int change_dir(char **cmd) {
    int rval = -1;
    if (cmd[1]) {
        rval = chdir(cmd[1]);
    } else {
        uid_t user = getuid();
        struct passwd *pw = getpwuid(user);
        if (pw != NULL) {
            rval = chdir(pw->pw_dir);
        }
    }
    return rval;
}

void parse_args(int argc, char **argv) 
{
    int c;
    while ((c = getopt(argc, argv, "vh")) != -1) 
    {
        switch (c) {
            case 'v':
                printf("Version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
                exit(0);
                break;
            case 'h':
                printf("Usage: %s [-h]\n", argv[0]);
                exit(0);
                break;
            // case '?':
            //     if (isprint(optopt)) {
            //         fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            //     } else {
            //         fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            //     }
            //     exit(1);
            //     break;
            default:
                fprintf(stderr, "Usage: %s [-h]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    {
        /* code */
    }
    
}
