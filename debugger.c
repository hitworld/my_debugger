#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include "linenoise.h"

struct Debugger_Data {
    long long break_addrs[0x10];
    char break_datas[0x10];
} my_data;

void continue_execution(int pid) {
    int wait_status;
    int options = 0;

    ptrace(PTRACE_CONT, pid, NULL, NULL);
    waitpid(pid, &wait_status, options);
    printf("break");
    fflush(stdout);
}

void breakpoint_enable(int pid, long long value) {
    printf("pid: %d, addr: %p\n", pid, (void *)value);
    fflush(stdout);
    long data = ptrace(PTRACE_PEEKDATA, pid, value, NULL);
    my_data.break_addrs[0] = value;
    my_data.break_datas[0] = (data & 0xff);
    ptrace(PTRACE_POKEDATA, pid, value, ((data & ~0xff) | 0xcc));

    return;
}

void handle_command(int pid, char * str) {
    char * command = strtok(str, " \n");

    if (!strcmp(command, "c")) {
        continue_execution(pid);
        return;
    } else if (!strcmp(command, "b")) {
        char * expr = strtok(NULL, "\n");
        if(!expr) {
            printf("Error:b <expr>\n");
            return;
        }
        int len = strlen(expr);
        long long value = 0;
        if(strncmp("0x", expr, 2) == 0) {
            int i = 2;
            for(;(i < len) && ((expr[i] >= 0x30 && expr[i] <= 0x39) || (expr[i] >= 0x41 && expr[i] <= 0x5A) || (expr[i] >= 0x61 && expr[i] <= 0x7A));i++) {
                if(expr[i] >= 0x30 && expr[i] <= 0x39) {
                    value *= 0x10;
                    value += expr[i] - 0x30;
                } else if (expr[i] >= 0x41 && expr[i] <= 0x46) {
                    value *= 0x10;
                    value += expr[i] - 0x37;
                } else {
                    value *= 0x10;
                    value += expr[i] - 0x57;
                }
            }
            if(i != len) {
                printf("expr\n");
            }
        } else {
            int i = 0;
            for(;(i < len) && (expr[i] >= 0x30 && expr[i] <= 0x39);i++) {
                value *= 10;
                value += expr[i] - 0x30;
            }
            if(i != len) {
                printf("expr\n");
            }
        }
        breakpoint_enable(pid, value);
        return;
    } else if (!strcmp(command, "q")) {
        ptrace(PTRACE_KILL, pid, NULL, NULL);
        exit(0);
    }
    return;
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
