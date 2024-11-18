#include "subscriber.h"

void start_subscriber(int argc, char* argv[]) {
    char pipeSSC[255];
    char interests[10];
    int pid = getpid();  

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
        exit(EXIT_FAILURE);
    }
    interests[strcspn(interests, "\n")] = '\0';

    // this sends the interests to the server
    for (int i = 0; i < strlen(interests); i++) {
        char reg_msg[256];
        snprintf(reg_msg, sizeof(reg_msg), "%d S R %c", pid, interests[i]);
        int reg_fd = open("/tmp/sc_reg", O_WRONLY);
        if (reg_fd == -1) {
            perror("open");
            flog(LOG_ERROR, "Failed to open the register pipe\n");
            exit(EXIT_FAILURE);
        }
        if (write(reg_fd, reg_msg, strlen(reg_msg)) == -1) {
            perror("write");
            flog(LOG_ERROR, "Failed to write to the register pipe\n");
            close(reg_fd);
            exit(EXIT_FAILURE);
        }        
        close(reg_fd);
    }

    // flags
    flags_t* parser = flags_create();
    flags_string(parser, pipeSSC, 's', "/tmp/sc_sub");
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