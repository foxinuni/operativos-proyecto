#include "coms.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void sub_register(int fd, int pid, char* interest) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%d S R %s", pid, interest);
    write(fd, buffer, strlen(buffer));
}

void sub_unregister(int fd, int pid) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%d S U", pid);
    write(fd, buffer, strlen(buffer));
}

void pub_register(int fd, int pid) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%d P R", pid);
    write(fd, buffer, strlen(buffer));
}

void pub_unregister(int fd, int pid) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%d P U", pid);
    write(fd, buffer, strlen(buffer));
}