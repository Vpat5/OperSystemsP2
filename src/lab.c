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

void sh_destroy(struct shell *sh) {
    if (sh->prompt) {
        free(sh->prompt);
    }
    clear_history();
    exit(EXIT_SUCCESS);
    
}

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
            default:
                fprintf(stderr, "Usage: %s [-h]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
}
