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
#include <pthread.h>
#include <time.h>
#include <sys/time.h>


#include "args.h"
#include "debug.h"
#include "memory.h"

struct gengetopt_args_info args_info;

#define C_ERRO_PTHREAD_CREATE 1
#define C_ERRO_PTHREAD_JOIN 2
#define C_ERRO_MUTEX_INIT 3
#define C_ERRO_MUTEX_DESTROY 4
#define C_ERRO_CONDITION_INIT 5
#define C_ERRO_CONDITION_DESTROY 6
#define C_ERRO_MUTEX_LOCK 7
#define C_ERRO_MUTEX_UNLOCK 8
#define C_ERRO_CONDITION_WAIT 9
#define C_ERRO_CONDITION_SIGNAL 10
#define C_ERRO_GETTIMEOFDAY 11

#define NUM_ESTACOES 10

float estacoes[NUM_ESTACOES] = {
    89.0f, 90.2f, 91.7f, 92.6f, 92.8f,
    93.0f, 98.7f, 104.2f, 106.4f, 107.4f
};

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int estacao_atual;
    int pesquisar; /* 1: pesquisa pedida; 0: pode monitorizar */
} pthread_params_t;

void *thread_1(void *arg);
void *thread_2(void *arg);
void esperar_segundo(pthread_params_t *params);

int main(int argc, char *argv[]) {
    if(cmdline_parser(argc, argv, &args_info) != 0){
        return 1;
    }
    
    float station = estacoes[0];
    pthread_t threadSearch, threadMonitor;
    pthread_params_t threadParams;

    threadParams.estacao_atual = 0;
    threadParams.pesquisar = 0;

    if (args_info.station_given) {
        station = args_info.station_arg;

        if (!(station >= 87.5f && station <= 108.0f)) {
            printf("[ERROR] Invalid FM frequency.\n");
            cmdline_parser_free(&args_info);
            return 1;
        }

        threadParams.estacao_atual = -1;
        for (int i = 0; i < NUM_ESTACOES; i++) {
            if (station == estacoes[i]) {
                threadParams.estacao_atual = i;
                break;
            }
        }

        if (threadParams.estacao_atual == -1) {
            printf("[ERROR] Unknown radio station frequency %.1f.\n", station);
            cmdline_parser_free(&args_info);
            return 1;
        }
    }

    cmdline_parser_free(&args_info);

    if ((errno = pthread_mutex_init(&threadParams.mutex, NULL)) != 0){
        ERROR(C_ERRO_MUTEX_INIT, "pthread_mutex_init() failed!");
    }

    if ((errno = pthread_cond_init(&threadParams.cond, NULL)) != 0){
        ERROR(C_ERRO_CONDITION_INIT, "pthread_cond_init() failed!");
    }

    printf("[INFO] Tuning radio station %.1f...\n", station);
    fflush(stdout);

    if ((errno = pthread_create(&threadSearch, NULL, thread_1, &threadParams)) != 0){
        ERROR(C_ERRO_PTHREAD_CREATE, "pthread_create() failed!");
    }

    if ((errno = pthread_create(&threadMonitor, NULL, thread_2, &threadParams)) != 0){
        ERROR(C_ERRO_PTHREAD_CREATE, "pthread_create() failed!");
    }
    
    if ((errno = pthread_join(threadSearch, NULL)) != 0) {
        ERROR(C_ERRO_PTHREAD_JOIN, "pthread_join() failed!");
    }

    if ((errno = pthread_join(threadMonitor, NULL)) != 0) {
        ERROR(C_ERRO_PTHREAD_JOIN, "pthread_join() failed!");
    }

    if ((errno = pthread_cond_destroy(&threadParams.cond)) != 0) {
        ERROR(C_ERRO_CONDITION_DESTROY, "pthread_cond_destroy() failed!");
    }

    if ((errno = pthread_mutex_destroy(&threadParams.mutex)) != 0) {
        ERROR(C_ERRO_MUTEX_DESTROY, "pthread_mutex_destroy() failed!");
    }

    return 0;
}

/* Thread de pesquisa */
void *thread_1(void *arg) {
    pthread_params_t *params = (pthread_params_t *)arg;

    while (1) {
        if ((errno = pthread_mutex_lock(&params->mutex)) != 0) {
            ERROR(C_ERRO_MUTEX_LOCK, "pthread_mutex_lock() failed!");
        }

        /* Espera que a monitorizacao detete um sinal inferior a 50. */
        while (params->pesquisar == 0) {
            if ((errno = pthread_cond_wait(&params->cond, &params->mutex)) != 0) {
                ERROR(C_ERRO_CONDITION_WAIT, "pthread_cond_wait() failed!");
            }
        }

        esperar_segundo(params);
        params->estacao_atual = (params->estacao_atual + 1) % NUM_ESTACOES;
        printf("[SEARCH] Search radio frequency... %.1f.\n",
               estacoes[params->estacao_atual]);
        fflush(stdout);

        /* A nova estacao ja esta selecionada: pode voltar a monitorizar. */
        params->pesquisar = 0;
        if ((errno = pthread_cond_signal(&params->cond)) != 0) {
            ERROR(C_ERRO_CONDITION_SIGNAL, "pthread_cond_signal() failed!");
        }

        if ((errno = pthread_mutex_unlock(&params->mutex)) != 0) {
            ERROR(C_ERRO_MUTEX_UNLOCK, "pthread_mutex_unlock() failed!");
        }
    }

    return NULL;
}

/* Thread de monitorizacao */
void *thread_2(void *arg) {
    pthread_params_t *params = (pthread_params_t *)arg;
    unsigned int seed = time(NULL) ^ getpid() ^ pthread_self();

    while (1) {
        if ((errno = pthread_mutex_lock(&params->mutex)) != 0) {
            ERROR(C_ERRO_MUTEX_LOCK, "pthread_mutex_lock() failed!");
        }

        esperar_segundo(params);

        int qualidade = rand_r(&seed) % (100 + 1);
        printf("[MONITOR] Check signal quality... %d%%.\n", qualidade);
        fflush(stdout);

        if (qualidade < 50) {
            params->pesquisar = 1;
            if ((errno = pthread_cond_signal(&params->cond)) != 0) {
                ERROR(C_ERRO_CONDITION_SIGNAL, "pthread_cond_signal() failed!");
            }

            /* Liberta o mutex e espera que a pesquisa termine. */
            while (params->pesquisar == 1) {
                if ((errno = pthread_cond_wait(&params->cond, &params->mutex)) != 0) {
                    ERROR(C_ERRO_CONDITION_WAIT, "pthread_cond_wait() failed!");
                }
            }
        }

        if ((errno = pthread_mutex_unlock(&params->mutex)) != 0) {
            ERROR(C_ERRO_MUTEX_UNLOCK, "pthread_mutex_unlock() failed!");
        }
    }

    return NULL;
}

/* Chamar com o mutex adquirido; a espera liberta-o e readquire-o no fim.
 * Usa o mesmo calculo do prazo do exemplo Class5/lab5. */
void esperar_segundo(pthread_params_t *params) {
    struct timeval tp;
    struct timespec ts;

    if (gettimeofday(&tp, NULL) != 0) {
        ERROR(C_ERRO_GETTIMEOFDAY, "gettimeofday() failed!");
    }
    ts.tv_sec = tp.tv_sec + 1;
    ts.tv_nsec = tp.tv_usec * 1000;

    /* Um despertar antecipado nao deve encurtar o intervalo. */
    do {
        errno = pthread_cond_timedwait(&params->cond, &params->mutex, &ts);
        if (errno != 0 && errno != ETIMEDOUT) {
            ERROR(C_ERRO_CONDITION_WAIT, "pthread_cond_timedwait() failed!");
        }
    } while (errno != ETIMEDOUT);
}
