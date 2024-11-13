#include "subscriber.h"

void start_subscriber(int argc, char* argv[]) {
    char pipeSSC[255];

    flags_t* parser = flags_create();
    flags_string(parser, pipeSSC, 's', "/tmp/pipeSSC");

    flags_parse(parser, argc, argv);
    flags_destroy(parser);

    flog(LOG_INFO, "Subscriber initialized with pipe: %s\n", pipeSSC);

    int fd = open(pipeSSC, O_RDONLY);
    if (fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the pipe: %s\n", pipeSSC);
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    while (read(fd, buffer, sizeof(buffer)) > 0) {
        buffer[strcspn(buffer, "\n")] = '\0';
        flog(LOG_INFO, "Received news: %s\n", buffer);
    }

    close(fd);
    flog(LOG_INFO, "Subscriber finished\n");
}
