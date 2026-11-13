/**
 * @file main.c
 * @brief Description
 * @date 2018-1-1
 * @author name of author
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "debug.h"
#include "memory.h"
#include "args.h"

char *file = NULL;

void trata_sinal(int sinal);

int main(int argc, char *argv[]) {
	struct gengetopt_args_info args;
	
	if (cmdline_parser(argc, argv, &args) != 0) {
		return 1;
	}
	
	file = args.file_arg;
	
	pid_t pid = fork();
	if (pid == 0) {
		//código filho
		while(1) {
			sleep(5);
			if (kill(getppid(), SIGUSR1) < 0) {
				break;
			}
		}
	} else if (pid > 0) {
		//pai
		struct sigaction act;
		act.sa_handler = trata_sinal;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_RESTART;
		
		if (sigaction(SIGUSR1, &act, NULL) < 0) {
			ERROR(1, "sigaction() failed\n");
		}
		
		while(1) {
			pause();
		}
	} else {
		ERROR(1, "fork() failed\n");
	}


    cmdline_parser_free(&args);

    return 0;
}


void trata_sinal (int sinal){
    (void) sinal;

    // ver: getline()
    FILE *f = fopen(file, "r");

    if (f == NULL){
        WARNING("fopen() failed");
    } else{
        char line[200];
        
        if(fgets(line, 200, f) != NULL) {
            printf("Primeira linha: %s \n", line);
        }
        fclose(f);
    }
}