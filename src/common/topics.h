#ifndef SC_TOPICS_H
#define SC_TOPICS_H

typedef enum {
    TOPIC_ART = 'A',
    TOPIC_ENTERTAINMENT = 'E',
    TOPIC_SCIENCE = 'C',
    TOPIC_POLITICS = 'P',
    TOPIC_EVENTS = 'S',
} topic_t;

topic_t topic_from_string(const char* topic);

#endif