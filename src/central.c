#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>

#include "flags.h"
#include "commons.h"

static bool shutting_down = false;

// Server settings
static int ttl;
static char reg_pipe[255];
static char sub_pipe[255];
static char pub_pipe[255];

void sigint_handler(int _) {
    flog(LOG_INFO, "Received SIGINT, shutting down!\n");
    shutting_down = true;
}

void parse_arguments(int argc, char* argv[]) {
    static flags_t parser;

    // Define the flags being used
    flags_int(&parser, &ttl, 't', 30);
    flags_string(&parser, reg_pipe, 'r', "/tmp/sc_reg");
    flags_string(&parser, sub_pipe, 's', "/tmp/sc_sub");
    flags_string(&parser, pub_pipe, 'p', "/tmp/sc_pub");

    // Parse the arguments
    flags_parse(&parser, argc, argv);
}

void* register_thread(void* args) {
    flog(LOG_INFO, "Starting the register thread!\n");

    // Open the register pipe
    int fd = open(reg_pipe, O_RDONLY);
    if (fd == -1) {
        perror("open");

        flog(LOG_ERROR, "Failed to open the register pipe!\n");
        shutting_down = true;
        return NULL;
    }

    // File descriptor is set to non-blocking
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");

        flog(LOG_ERROR, "Failed to set the register pipe to non-blocking!\n");
        shutting_down = true;
        return NULL;
    }

    // Read the register pipe
    while (!shutting_down) {
        const int buffer_size = 256;
        char buffer[buffer_size];
        int err;

        if ((err = read(fd, buffer, buffer_size)) == -1) {
            perror("read");

            flog(LOG_ERROR, "Failed to read the action from the register pipe!\n");
            shutting_down = true;
            return NULL;
        } else if (err == 0) {
            continue;
        }

        // Parse the action
        int pid;
        char system_type, action_type, topic;
        sscanf(buffer, "%d %c %c %c",  &pid, &system_type, &action_type, &topic);

        // Print the parsed action
        switch (action_type) {
            case 'R':
                flog(LOG_INFO, "Registering system %d with topic %c\n", pid, topic);
                break;
            case 'U':
                flog(LOG_INFO, "Unregistering system %d\n", pid);
                break;
            default:
                flog(LOG_WARNING, "Unknown action type: %c\n", action_type);
                break;
        }
    }

    // Close the register pipe
    close(fd);

    // Shutdown thread
    flog(LOG_INFO, "Register thread shutting down!\n");
    
    return NULL;
}

int main(int argc, char* argv[]) {
    pthread_t register_th;

    // Parse the arguments
    parse_arguments(argc, argv); 

    // Print the parsed arguments
    flog(
        LOG_INFO, 
        "Initializing central server with: (ttl: %d, reg: \"%s\", pub: \"%s\", sub: \"%s\")\n", 
        ttl, reg_pipe, pub_pipe, sub_pipe
    );

    // Register signals
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal");

        flog(LOG_ERROR, "Failed to register the SIGINT handler!\n");
        return 1;
    }

    // Create the semaphores
    if (pthread_create(&register_th, NULL, register_thread, NULL) != 0) {
        perror("pthread_create");
        
        flog(LOG_ERROR, "Failed to create the register thread!\n");
        return 1;
    }

    // Wait for the register thread to finish
    pthread_join(register_th, NULL);

    return 0;
}