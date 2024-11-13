#include "subscriber.h"

void start_subscriber(int argc, char* argv[]) {
    char pipeSSC[255];

    // this helps to read the arguments, same than publishers
    flags_t* parser = flags_create();
    flags_string(parser, pipeSSC, 's', "/tmp/pipeSSC");

    flags_parse(parser, argc, argv);
    flags_destroy(parser);

    flog(LOG_INFO, "Subscriber initialized with pipe: %s\n", pipeSSC);

    // pipe
    int fd = open(pipeSSC, O_RDONLY);
    if (fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the pipe: %s\n", pipeSSC);
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    // if the buffer is > 0 it took information, if is equal to 0 is the end of the file and if it´s equal to -1 is an error
    while (read(fd, buffer, sizeof(buffer)) > 0) {
        buffer[strcspn(buffer, "\n")] = '\0';
        flog(LOG_INFO, "Received news: %s\n", buffer);
    }

    close(fd);
    flog(LOG_INFO, "Subscriber finished\n");
}

int main(int argc, char* argv[]) {
    start_subscriber(argc, argv);
    return 0;
}