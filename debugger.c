#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include "linenoise.h"

void continue_execution(int pid) {
    int wait_status;
    int options = 0;

    ptrace(PTRACE_CONT, pid, NULL, NULL);
    waitpid(pid, &wait_status, options);
}

void breakpoint_enable(int pid) {
    printf("%d", pid);
}

void handle_command(int pid, char * str) {
    char * command = strtok(str, " \n");

    if (!strcmp(command, "c")) {
        continue_execution(pid);
    } else if (!strcmp(command, "b")) {
        char * expr = strtok(NULL, "\n");
        puts(expr);
        breakpoint_enable(pid);
    } else if (!strcmp(command, "q")) {
        ptrace(PTRACE_KILL, pid, NULL, NULL);
        exit(0);
    }
}

int main(int argc, char * argv[]) {
    if (argc < 2) {
        printf("Please provide the name of the program being debugged!");
        exit(-1);
    }
    char * prog_name = argv[1];

    int pid = fork();
    if (pid == 0) {
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
            printf("Error in ptrace\n");
            return -1;
        }
        execl(prog_name, prog_name, NULL);
    }
    else if (pid >= 1)  {
        int wait_status;
        int options = 0;
        waitpid(pid, &wait_status, options);

        char * line = NULL;
        while ((line = linenoise("cws_gdb> ")) != NULL) {
            handle_command(pid, line);
            linenoiseHistoryAdd(line);
            linenoiseFree(line);
        }
    }
}
