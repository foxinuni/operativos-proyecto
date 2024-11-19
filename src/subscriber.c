#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "commons.h"
#include "coms.h"
#include "flags.h"

#define DELAY_TIME 50000 // 50 ms (50000 us)

enum subscriber_state {
    STATE_INIT,
    STATE_CONNECTED,
    STATE_TERMINATED
} state;

sem_t awknowledged;

// arguments
char reg_pipe[255];
char sub_pipe[255];

// client settings
char interests[10];
int reg_fd, sub_fd;
int pid;

void signal_awknowledged() {
    sem_post(&awknowledged);
}

void signal_terminate() {
    flog(LOG_INFO, "Recieved terminate signal from central\n");
    state = STATE_TERMINATED;
    sem_post(&awknowledged);
}

void signal_shutdown() {
    flog(LOG_INFO, "Recieved shutdown signal\n");
    state = STATE_TERMINATED;
    sem_post(&awknowledged);
}

int await_ack() {
    sem_wait(&awknowledged);
    return state == STATE_TERMINATED;
}

void* event_loop(void* _) {
    flog(LOG_INFO, "Initiating the event loop...\n");

    for (;;) {
        switch (state) {
        case STATE_INIT:
            // register
            sub_register(reg_fd, pid, interests);
            if (await_ack() == 1) {
                flog(LOG_ERROR, "Failed to register to server!");
                continue;
            }

            flog(LOG_INFO, "Registered with interests %s\n", interests);
            state = STATE_CONNECTED;
            break;
        case STATE_CONNECTED: {
            char buffer[256];
            int bytes_read = read(sub_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read == -1) {
                if (errno == EAGAIN) {
                    continue;
                }

                perror("read");
                flog(LOG_ERROR, "Failed to read from the sub pipe!\n");
                state = STATE_TERMINATED;
                continue;
            }

            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                flog(LOG_INFO, "Received message: %s\n", buffer);
            }
        }
        break;
        case STATE_TERMINATED:
            flog(LOG_WARNING, "Process terminated- deregistering from the central.\n");
            sub_unregister(reg_fd, pid);

            return NULL; // exit
        }

        usleep(DELAY_TIME);
    }
}

void parse_arguments(int argc, char* argv[]) {
    static flags_t parser;

    // Define the flags being used
    flags_string(&parser, reg_pipe, 'c', "/tmp/sc_reg");

    // Parse the arguments
    flags_parse(&parser, argc, argv);
}

int main(int argc, char* argv[]) {
    pthread_t event_loop_th;

    // parse arguments
    parse_arguments(argc, argv);

    // select the interests
    printf("Seleccione los tipos de noticias que desea recibir ingresando las letras correspondientes.\n");
    printf("Opciones disponibles:\n");
    printf("  P - Política\n");
    printf("  A - Arte\n");
    printf("  C - Ciencia\n");
    printf("  E - Farándula y Espectáculos\n");
    printf("  S - Sucesos\n");
    printf("Ingrese sus intereses (ejemplo: 'PACS' para Política, Arte, Ciencia y Sucesos): ");

    if (fgets(interests, sizeof(interests), stdin) == NULL) {
        flog(LOG_ERROR, "Error al leer los intereses del usuario.\n");
        return -1;
    }

    interests[strcspn(interests, "\n")] = '\0';

    // get the pid
    pid = getpid();

    // set pipe name
    snprintf(sub_pipe, sizeof(sub_pipe), "/tmp/sc_sub_%d", pid);

    // init semaphore
    if (sem_init(&awknowledged, 0, 0) == -1) {
        perror("sem_init");
        flog(LOG_ERROR, "Failed to initialize the semaphore!\n");
        return -1;
    }

    // register usr1 signal
    if (signal(SIGUSR1, signal_awknowledged) == SIG_ERR) {
        perror("signal");
        flog(LOG_ERROR, "Failed to register the signal!\n");
        return -1;
    }

    // register usr2 signal
    if (signal(SIGUSR2, signal_terminate) == SIG_ERR) {
        perror("signal");
        flog(LOG_ERROR, "Failed to register the terminate signal!\n");
        return -1;
    }

    // register the sigint sifnal
    if (signal(SIGINT, signal_shutdown) == SIG_ERR) {
        perror("signal");
        flog(LOG_ERROR, "Failed to register the shutdown signal!\n");
        return -1;
    }

    // create the pipe
    if (mkfifo(sub_pipe, 0666) == -1) {
        perror("mkfifo");
        flog(LOG_ERROR, "Failed to create the sub pipe!\n");
        return -1;
    }

    // open pipes
    reg_fd = open(reg_pipe, O_WRONLY);
    if (reg_fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the register pipe!\n");
        return -1;
    }

    // open the pipe
    sub_fd = open(sub_pipe, O_RDONLY | O_NONBLOCK);
    if (sub_fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the sub pipe!\n");
        return -1;
    }

    // create threads
    if (pthread_create(&event_loop_th, NULL, event_loop, NULL) != 0) {
        perror("pthread_create");
        flog(LOG_ERROR, "Failed to create the working thread!\n");
        close(reg_fd);
        return -1;
    }

    // start the working thread
    pthread_join(event_loop_th, NULL);

    // close pipes
    close(reg_fd);
    close(sub_fd);
    unlink(sub_pipe);

    return 0;
}