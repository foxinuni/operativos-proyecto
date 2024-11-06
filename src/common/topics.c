#include "topics.h"

topic_t topic_from_string(const char* topic) {
    switch (topic[0]) {
        case 'A':
            return TOPIC_ART;
        case 'E':
            return TOPIC_ENTERTAINMENT;
        case 'C':
            return TOPIC_SCIENCE;
        case 'P':
            return TOPIC_POLITICS;
        case 'S':
            return TOPIC_EVENTS;
        default:
            return -1;
    }
}
