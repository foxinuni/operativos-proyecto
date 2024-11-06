/**
 * @file topics.h
 * @brief Defines the different topics available
 * @date 2024-11-05
 *
 * The following file contains the definition of the different topics available.
 * Topics are used to categorize the different messages sent by the publishers.
 */
#ifndef LSC_TOPICS_H
#define LSC_TOPICS_H

typedef enum {
    TOPIC_ART = 'A',
    TOPIC_ENTERTAINMENT = 'E',
    TOPIC_SCIENCE = 'C',
    TOPIC_POLITICS = 'P',
    TOPIC_EVENTS = 'S',
} topic_t;

topic_t topic_from_string(const char* topic);

#endif