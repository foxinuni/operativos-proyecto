#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h> 
#include <time.h>

#include "flags.h"
#include "commons.h"

#define LOOP_DELAY 100000
#define MAX_SUBSCRIBERS 10
#define MAX_PUBLISHERS 10
#define MAX_TOPICS 10

static bool shutting_down = false;

// server settings
static int ttl;
static char reg_pipe[255];
static char pub_pipe[255];

// array of subscribers and matrics of subscribers and their interests
static int subscriber_pids[MAX_SUBSCRIBERS];
static int subscribers_fd[MAX_SUBSCRIBERS];
static char subscriber_interests[MAX_SUBSCRIBERS][MAX_TOPICS];
static int subscriber_count = 0;
pthread_mutex_t subscriber_mutex = PTHREAD_MUTEX_INITIALIZER;

// publishers
static int publisher_pids[MAX_PUBLISHERS];
static int publisher_count = 0;
pthread_mutex_t publisher_mutex = PTHREAD_MUTEX_INITIALIZER;

// pipes
int reg_fd, pub_fd;

void parse_arguments(int argc, char* argv[]) {
    static flags_t parser;

    // Define the flags being used
    flags_int(&parser, &ttl, 't', 30);
    flags_string(&parser, reg_pipe, 'c', "/tmp/sc_reg");
    flags_string(&parser, pub_pipe, 'p', "/tmp/sc_pub");

    // Parse the arguments
    flags_parse(&parser, argc, argv);
}

void register_subscriber(int pid, char* topics) {
    flog(LOG_INFO, "Registering subscriber %d with topics: %s\n", pid, topics);

    // lock the subscriber's array
    pthread_mutex_lock(&subscriber_mutex);

    // find the subscriber's PID
    for (int i = 0; i < subscriber_count; i++) {
        if (subscriber_pids[i] == pid) {
            // update the subscriber's interests
            strncpy(subscriber_interests[i], topics, MAX_TOPICS - 1);
            subscriber_interests[i][MAX_TOPICS - 1] = '\0'; // Null-terminate
            
            // aknowledge the registration
            kill(pid, SIGUSR1);
            return;
        }
    }

    // store the subscriber's PID and interests in the array
    if (subscriber_count < MAX_SUBSCRIBERS) {
        subscriber_pids[subscriber_count] = pid;
        strncpy(subscriber_interests[subscriber_count], topics, MAX_TOPICS - 1);
        subscriber_interests[subscriber_count][MAX_TOPICS - 1] = '\0'; // Null-terminate

        // aknowledge the registration
        kill(pid, SIGUSR1);

        // generate the subscriber's pipe name
        char pipe_name[255];
        snprintf(pipe_name, sizeof(pipe_name), "/tmp/sc_sub_%d", pid);

        // open the subscriber's pipe
        subscribers_fd[subscriber_count] = open(pipe_name, O_WRONLY);
        if (subscribers_fd[subscriber_count] == -1) {
            perror("open");
            flog(LOG_ERROR, "Failed to open the sub pipe!\n");
            kill(pid, SIGUSR2);
        }

        subscriber_count++;
    } else {
        flog(LOG_WARNING, "Maximum number of subscribers reached\n");
        kill(pid, SIGUSR2);
    }

    // unlock the subscriber's array
    pthread_mutex_unlock(&subscriber_mutex);
}

void register_publisher(int pid) {
    flog(LOG_INFO, "Registering publisher %d\n", pid);

    // lock the publisher's array
    pthread_mutex_lock(&publisher_mutex);

    // find the publisher's PID
    for (int i = 0; i < publisher_count; i++) {
        if (publisher_pids[i] == pid) {
            kill(pid, SIGUSR1); // aknowledge the registration
            return;
        }
    }

    // store the publisher's PID in the array
    if (publisher_count < MAX_PUBLISHERS) {
        publisher_pids[publisher_count] = pid;

        // aknowledge the registration
        kill(pid, SIGUSR1);
        publisher_count++;
    } else {
        flog(LOG_WARNING, "Maximum number of publishers reached\n");
        kill(pid, SIGUSR2);
    }

    // unlock the publisher's array
    pthread_mutex_unlock(&publisher_mutex);
}

void deregister_subscriber(int pid) {
    flog(LOG_INFO, "Deregistering subscriber %d\n", pid);

    // lock the subscriber's array
    pthread_mutex_lock(&subscriber_mutex);

    // find the subscriber's PID
    for (int i = 0; i < subscriber_count; i++) {
        if (subscriber_pids[i] == pid) {
            // remove the subscriber from the array
            memmove(&subscriber_pids[i], &subscriber_pids[i + 1], (subscriber_count - i - 1) * sizeof(int));
            memmove(&subscribers_fd[i], &subscribers_fd[i + 1], (subscriber_count - i - 1) * sizeof(int));
            memmove(&subscriber_interests[i], &subscriber_interests[i + 1], (subscriber_count - i - 1) * sizeof(subscriber_interests[0]));
            subscriber_count--;
           
            // close the subscriber's pipe
            close(subscribers_fd[i]);
            break;
        }
    }

    // unlock the subscriber's array
    pthread_mutex_unlock(&subscriber_mutex);
}

void deregister_publisher(int pid) {
    flog(LOG_INFO, "Deregistering publisher %d\n", pid);

    // lock the publisher's array
    pthread_mutex_lock(&publisher_mutex);

    // find the publisher's PID
    for (int i = 0; i < publisher_count; i++) {
        if (publisher_pids[i] == pid) {
            // remove the publisher from the array
            memmove(&publisher_pids[i], &publisher_pids[i + 1], (publisher_count - i - 1) * sizeof(int));
            publisher_count--;
            break;
        }
    }

    // aknowledge the deregistration
    kill(pid, SIGUSR1);

    // unlock the publisher's array
    pthread_mutex_unlock(&publisher_mutex);
}

void deregister_and_kill_all() {
    // deregister all subscribers
    flog(LOG_INFO, "Deregistering all subscribers\n");

    for (int i = 0; i < subscriber_count; i++) {
        deregister_subscriber(subscriber_pids[i]);
        kill(subscriber_pids[i], SIGUSR2);
    }

    flog(LOG_INFO, "Deregistering all publishers\n");
    
    // deregister all publishers
    for (int i = 0; i < publisher_count; i++) {
        deregister_publisher(publisher_pids[i]);
        kill(publisher_pids[i], SIGUSR2);
    }
}

void process_request(pid_t pid, char system_type, char action_type, char* interest) {
    flog(LOG_INFO, "Received request from %d: %c %c %s\n", pid, system_type, action_type, interest);

    switch (action_type) {
    case 'R':
        switch (system_type) {
        case 'S':
            register_subscriber(pid, interest);
            break;
        case 'P':
            register_publisher(pid);
            break;
        default:
            flog(LOG_WARNING, "Unknown system type: %c\n", system_type);
            deregister_subscriber(pid);
            kill(pid, SIGUSR2);
            break;
        }

        break;
    case 'U':
        switch (system_type) {
        case 'S':
            deregister_subscriber(pid);
            break;
        case 'P':
            deregister_publisher(pid);
            break;
        default:
            flog(LOG_WARNING, "Unknown system type: %c\n", system_type);
            deregister_subscriber(pid);
            deregister_publisher(pid);
            kill(pid, SIGUSR2);
            break;
        }

        break;
    default:
        flog(LOG_WARNING, "Unknown action type: %c\n", action_type);
        deregister_subscriber(pid);
        kill(pid, SIGUSR2);
    } 
}

void* register_thread(void* args) {
    flog(LOG_INFO, "Starting the register thread!\n");

    char buffer[256];
    while (!shutting_down) {
        // read from the server-to-client pipe
        int bytes_read = read(reg_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read == -1) {
            if (errno == EAGAIN) {
                continue;
            }

            perror("read");
            flog(LOG_ERROR, "Failed to read from the register pipe\n");
            continue;
        }

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';

            // parse the message
            char system_type, action_type, interest[25] = {0};
            pid_t pid;

            sscanf(buffer, "%d %c %c %s", &pid, &system_type, &action_type, interest);
            
            // process request
            process_request(pid, system_type, action_type, interest);
        }

        usleep(LOOP_DELAY);
    }

    flog(LOG_INFO, "Register thread shutting down!\n");

    return NULL;
}


void* broadcast_thread(void* _) {
    flog(LOG_INFO, "Starting the broadcasting thread!\n");

    char buffer[256];
    while (!shutting_down) {
        ssize_t bytes_read = read(pub_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read == -1) {
            if (errno == EAGAIN) {
                continue;
            }

            perror("read");
            flog(LOG_ERROR, "Failed to read from the pub pipe\n");
            continue;
        }

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';  // Null-terminate the string

            // the first character represents the topic
            char topic = buffer[0];
            flog(LOG_INFO, "Received message with topic %c: %s\n", topic, buffer);

            // verify if unless one subcriptor is interested in the topic
            for (int i = 0; i < subscriber_count; i++) {
                bool should_broadcast = false;
                for (int j = 0; j < MAX_TOPICS; j++) {
                    if (subscriber_interests[i][j] == topic) {
                        should_broadcast = true;
                        break;
                    }
                }

                if (should_broadcast) {
                    // send the message to the subscriber
                    ssize_t bytes_written = write(subscribers_fd[i], buffer, bytes_read);
                    if (bytes_written == -1) {
                        perror("write");
                        flog(LOG_ERROR, "Failed to write to the sub pipe\n");
                        continue;
                    }
                }
            }
        }
    }

    flog(LOG_INFO, "Broadcasting thread shutting down!\n");
    return NULL;
}

void* ttl_thread(void* _) {
    clock_t start_time;
    bool in_ttl = false;
    
    while(!shutting_down) {
        if (in_ttl) {
            if (publisher_count > 0) {
                in_ttl = false;
            } else {
                clock_t current_time = clock();

                if ((current_time - start_time) / CLOCKS_PER_SEC >= ttl) {
                    flog(LOG_INFO, "TTL expired, shutting down!\n");

                    // deregister all
                    deregister_and_kill_all();

                    // set shutdown flag
                    shutting_down = true;
                }
            }
        } else {
            if (publisher_count == 0) {
                in_ttl = true;
                start_time = clock();
            }
        }

        usleep(LOOP_DELAY);
    }

    return NULL;
}

void sigint_handler(int _) {
    flog(LOG_INFO, "Received SIGINT, shutting down!\n");

    // deregister all
    deregister_and_kill_all();

    // set shutdown flag
    shutting_down = true;
}

int main(int argc, char* argv[]) {
    pthread_t register_th, broadcast_th, ttl_th;

    // parse arguments
    parse_arguments(argc, argv);
    flog(LOG_INFO, "Initializing central server with: (ttl: %d, reg: \"%s\", pub: \"%s\")\n", ttl, reg_pipe, pub_pipe);

    // register signal
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal");
        flog(LOG_ERROR, "Failed to register the SIGINT handler!\n");

        return 1;
    }

    // create pipes
    reg_fd = open(reg_pipe, O_RDONLY | O_NONBLOCK);
    if (reg_fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the register pipe!\n");

        return 1;
    }

    pub_fd = open(pub_pipe, O_RDONLY | O_NONBLOCK);
    if (pub_fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the pub pipe!\n");

        return 1;
    }

    // create threads
    if (pthread_create(&ttl_th, NULL, ttl_thread, NULL) != 0) {
        perror("pthread_create");
        flog(LOG_ERROR, "Failed to create the TTL thread!\n");

        close(reg_fd);
        close(pub_fd);
        return 1;
    }

    if (pthread_create(&broadcast_th, NULL, broadcast_thread, NULL) != 0) {
        perror("pthread_create");
        flog(LOG_ERROR, "Failed to create the pub-sub thread!\n");

        close(reg_fd);
        close(pub_fd);
        return 1;
    }

    if (pthread_create(&register_th, NULL, register_thread, NULL) != 0) {
        perror("pthread_create");
        flog(LOG_ERROR, "Failed to create the register thread!\n");

        close(reg_fd);
        close(pub_fd);
        return 1;
    }
    
    // join threads
    pthread_join(register_th, NULL);
    pthread_join(broadcast_th, NULL);
    pthread_join(ttl_th, NULL);

    // close pipes
    close(reg_fd);
    close(pub_fd);

    return 0;
}