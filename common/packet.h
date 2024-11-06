#define LSC_PACKET_H
/**
 * I had this really cool system in mind for data transfering
 * kinda based on how MQTT or Websockets work, but instead society 
 * wants me to do it differently it seems :( - Fair enough.
 * 
 * This header will probably be deleted later. For now, it is disabled.
 */

/**
 * @file packet.h
 * @brief Packet handling functions
 * @date 2024-11-05
 * 
 * The following file contains functions for the handling, and wrapping of packets.
 * Packets are containers for data sent between publishers and subscribers, and handled
 * by the broker.
 */

#ifndef LSC_PACKET_H
#define LSC_PACKET_H
#include <string.h>

typedef struct {
    size_t length; // The length of the packet
    char* topic; // The topic of the packet
    void* data; // The data of the packet
} packet_t;

// El paquete se crea con el tópico y los datos
// Despues de procesar, el dueño del paquete debe destriurlo usando packet_destroy.
packet_t* packet_create(const char* topic, const void* data, size_t length);

// Destruye el paquete y libera la memoria
void packet_destroy(packet_t* packet);

#endif