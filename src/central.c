#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h> 
#include "flags.h"
#include "commons.h"

#define MAX_SUBSCRIBERS 10
#define MAX_TOPICS 10

static bool shutting_down = false;

// Server settings
static int ttl;
static char reg_pipe[255];
static char sub_pipe[255];
static char pub_pipe[255];
// array of subscribers and matrics of subscribers and their interests
int subscriber_pids[MAX_SUBSCRIBERS];
char subscriber_interests[MAX_SUBSCRIBERS][MAX_TOPICS]; 
int subscriber_count = 0;

void register_subscriber(int pid, const char* topics) {
    // store the subscriber's PID and interests in the array
    if (subscriber_count < MAX_SUBSCRIBERS) {
        subscriber_pids[subscriber_count] = pid;
        strncpy(subscriber_interests[subscriber_count], topics, MAX_TOPICS - 1);
        subscriber_interests[subscriber_count][MAX_TOPICS - 1] = '\0'; // Null-terminate
        subscriber_count++;
    }
}

void sigint_handler(int _) {
    flog(LOG_INFO, "Received SIGINT, shutting down!\n");
    shutting_down = true;
    exit(0);
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

    int fd = open(reg_pipe, O_RDONLY);
    if (fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the register pipe!\n");
        shutting_down = true;
        return NULL;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        flog(LOG_ERROR, "Failed to set the register pipe to non-blocking!\n");
        shutting_down = true;
        return NULL;
    }

    char buffer[256];
    while (!shutting_down) {
        int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';

            int pid;
            char system_type, action_type, topic;
            sscanf(buffer, "%d %c %c %c", &pid, &system_type, &action_type, &topic);

            if (action_type == 'R') {
                flog(LOG_INFO, "Registering system %d with topic %c\n", pid, topic);

                // add topic to the subscriber's interests
                bool found = false;
                for (int i = 0; i < subscriber_count; i++) {
                    if (subscriber_pids[i] == pid) {
                        strncat(subscriber_interests[i], &topic, 1);
                        found = true;
                        break;
                    }
                }
                if (!found && subscriber_count < MAX_SUBSCRIBERS) {
                    subscriber_pids[subscriber_count] = pid;
                    snprintf(subscriber_interests[subscriber_count], MAX_TOPICS, "%c", topic);
                    subscriber_count++;
                }
            } else if (action_type == 'U') {
                flog(LOG_INFO, "Unregistering system %d\n", pid);
            
            } else {
                flog(LOG_WARNING, "Unknown action type: %c\n", action_type);
            }
        }
    }

    close(fd);
    flog(LOG_INFO, "Register thread shutting down!\n");

    return NULL;
}


void* pub_sub_thread(void* args) {
    flog(LOG_INFO, "Starting the pub-sub thread!\n");

    int pub_fd = open(pub_pipe, O_RDONLY);
    if (pub_fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the pub pipe!\n");
        shutting_down = true;
        return NULL;
    }

    int sub_fd = open(sub_pipe, O_WRONLY);
    if (sub_fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the sub pipe!\n");
        close(pub_fd);
        shutting_down = true;
        return NULL;
    }

    char buffer[256];
    while (!shutting_down) {
        ssize_t bytes_read = read(pub_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';  // Null-terminate the string

            // the first character represents the topic
            char topic = buffer[0];
            flog(LOG_INFO, "Received message with topic %c: %s\n", topic, buffer);

            // verify if unless one subcriptor is interested in the topic
            bool should_forward = false;
            for (int i = 0; i < subscriber_count; i++) {
                if (strchr(subscriber_interests[i], topic) != NULL) {
                    should_forward = true;
                    break;
                }
            }

            if (should_forward) {
                if (write(sub_fd, buffer, bytes_read) == -1) {
                    perror("write");
                    flog(LOG_ERROR, "Failed to write to the sub pipe\n");
                    break;
                }
                flog(LOG_INFO, "Forwarded to subscribers: %s\n", buffer);
            } else {
                flog(LOG_INFO, "No subscribers interested in topic %c, message skipped\n", topic);
            }
        }
    }

    close(pub_fd);
    close(sub_fd);
    flog(LOG_INFO, "Pub-sub thread shutting down!\n");

    return NULL;
}


int main(int argc, char* argv[]) {
    pthread_t register_th, pub_sub_th;

    parse_arguments(argc, argv);

    flog(LOG_INFO, "Initializing central server with: (ttl: %d, reg: \"%s\", pub: \"%s\", sub: \"%s\")\n", ttl, reg_pipe, pub_pipe, sub_pipe);

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal");
        flog(LOG_ERROR, "Failed to register the SIGINT handler!\n");
        return 1;
    }

    if (pthread_create(&register_th, NULL, register_thread, NULL) != 0) {
        perror("pthread_create");
        flog(LOG_ERROR, "Failed to create the register thread!\n");
        return 1;
    }

    if (pthread_create(&pub_sub_th, NULL, pub_sub_thread, NULL) != 0) {
        perror("pthread_create");
        flog(LOG_ERROR, "Failed to create the pub-sub thread!\n");
        return 1;
    }

    pthread_join(register_th, NULL);
    pthread_join(pub_sub_th, NULL);

    return 0;
}