/*
#include "packet.h"
#include <stdlib.h>
#include <string.h>

packet_t* packet_create(const char* topic, const void* data, size_t length) {
    // Se calcula el tamaño del paquete
    packet_t* packet = malloc(sizeof(packet_t));
    size_t topic_length = strlen(topic);
    size_t packet_size = sizeof(packet_t) + topic_length + 1 + length;

    // Se reserva memoria para la estructura
    packet->length = packet_size;

    // Se reserva memoria para el topic
    packet->topic = malloc(strlen(topic) + 1);
    strcpy(packet->topic, topic);

    // Se reserva memoria para los datos
    packet->data = malloc(length);
    memcpy(packet->data, data, length);

    return packet;
}

void packet_destroy(packet_t* packet) {
    free(packet->topic);
    free(packet->data);
    free(packet);
}
*/