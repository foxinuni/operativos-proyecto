#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <pthread.h>

#include "commons.h"
#include "flags.h"
#include "coms.h"

#define DELAY_TIME 50000 // 50 ms (50000 us)

enum publisher_state {
    STATE_INIT,
    STATE_TRANSMITTING,
    STATE_TERMINATED
} state;

sem_t awknowledged;

// client settings
int reg_fd, pub_fd;
FILE* fp;

// arguments
char reg_pipe[255];
char pub_pipe[255];
char file[255];
int ttl;
pid_t pid;

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
            pub_register(reg_fd, pid);
            if (await_ack() == 1) {
                flog(LOG_ERROR, "Failed to register to server!");
                continue;
            }

            flog(LOG_INFO, "Registered to central successfully.\n");
            state = STATE_TRANSMITTING;
            break;
        case STATE_TRANSMITTING: {
            char buffer[256];

            // Get line from file
            if (fgets(buffer, sizeof(buffer), fp) == NULL) {
                flog(LOG_INFO, "End of file reached\n");
                state = STATE_TERMINATED;
                continue;
            }

            if (strlen(buffer) > 1 && buffer[strlen(buffer) - 1] == '\n') {
                buffer[strlen(buffer) - 1] = '\0';

                // send message
                if (write(pub_fd, buffer, strlen(buffer)) == -1) {
                    perror("write");
                    flog(LOG_ERROR, "Failed to write to the pub pipe!\n");
                    state = STATE_TERMINATED;
                }

                flog(LOG_INFO, "Sent message: %s\n", buffer);
            }

            sleep(ttl);             
        }
        break;
        case STATE_TERMINATED:
            flog(LOG_WARNING, "Process terminated- deregistering from the central.\n");
            pub_unregister(reg_fd, pid);

            return NULL; // exit
        }

        if (state != STATE_TRANSMITTING) {
            usleep(DELAY_TIME);
        }
    }
}

void parse_arguments(int argc, char* argv[]) {
    static flags_t parser;

    // Define the flags being used
    flags_string(&parser, reg_pipe, 'p', "/tmp/sc_reg");
    flags_string(&parser, pub_pipe, 'p', "/tmp/sc_pub");
    flags_string(&parser, file, 'f', "");
    flags_int(&parser, &ttl, 't', 0);
    flags_parse(&parser, argc, argv);

    // validate file was passed
    if (strlen(file) == 0) {
        flog(LOG_ERROR, "Missing file argument\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    pthread_t event_loop_th;

    // Parse arguments
    parse_arguments(argc, argv);
    flog(LOG_INFO, "Publisher initialized with (reg: \"%s\", pub: \"%s\", file: \"%s\", time: %d seconds)\n", reg_pipe, pub_pipe, file, ttl);

    // Get pid
    pid = getpid();

    // Create the semaphore
    if (sem_init(&awknowledged, 0, 0) == -1) {
        perror("sem_init");
        flog(LOG_ERROR, "Failed to create the semaphore\n");
        exit(EXIT_FAILURE);
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

    // Open file 
    fp = fopen(file, "r");
    if (!fp) {
        flog(LOG_ERROR, "Failed to open the news file: %s\n", file);
        exit(EXIT_FAILURE);
    }

    // Create pipes
    reg_fd = open(reg_pipe, O_WRONLY);
    if (reg_fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the pipe: %s\n", reg_pipe);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    pub_fd = open(pub_pipe, O_WRONLY);
    if (pub_fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the pipe: %s\n", pub_pipe);
        fclose(fp);
        exit(EXIT_FAILURE);
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

    close(reg_fd);
    close(pub_fd);

    return 0;
}
