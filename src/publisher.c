#include "publisher.h"

void start_publisher(int argc, char* argv[]) {
    char pipePSC[255];
    char file[255];
    int timeN;

    // this helps to read the arguments in the shell with the specific flags
    flags_t* parser = flags_create();
    flags_string(parser, pipePSC, 'p', "/tmp/pipePSC");
    flags_string(parser, file, 'f', "news.txt");
    flags_int(parser, &timeN, 't', 1);

    flags_parse(parser, argc, argv);
    flags_destroy(parser);

    flog(LOG_INFO, "Publisher initialized with pipe: %s, file: %s, time interval: %d seconds\n", pipePSC, file, timeN);

    // file
    FILE* fp = fopen(file, "r");
    if (!fp) {
        flog(LOG_ERROR, "Failed to open the news file: %s\n", file);
        exit(EXIT_FAILURE);
    }

    // pipe
    int fd = open(pipePSC, O_WRONLY);
    if (fd == -1) {
        perror("open");
        flog(LOG_ERROR, "Failed to open the pipe: %s\n", pipePSC);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strlen(line) > 1 && line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }

        // write the line content in the pipe
        if (write(fd, line, strlen(line)) == -1) {
            perror("write");
            flog(LOG_ERROR, "Failed to write to the pipe\n");
            break;
        }
        flog(LOG_INFO, "Published: %s\n", line);
        sleep(timeN);
    }

    close(fd);
    fclose(fp);
    flog(LOG_INFO, "Publisher finished\n");
}

int main(int argc, char* argv[]) {
    start_publisher(argc, argv);
    return 0;
}
