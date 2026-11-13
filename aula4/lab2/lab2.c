#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include "debug.h"

int main(int argc, char *argv[]) {
    /* Silencia os warnings */
    (void)argc;
    (void)argv;

    printf("Nome do programa: %s \n", argv[0]);
    printf("PID do processo atual: %d \n", getpid());
    printf("Numero de threads do processo atual: ");
    fflush(stdout);

    char cmd[200];
    sprintf(cmd, "ps -efL | grep %s | grep -v grep | wc -l", argv[0]);
    system(cmd);

    exit(0);
}
