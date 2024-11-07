#include "flags.h"
#include <stdlib.h>
#include <string.h>

flags_t* flags_create() {
    flags_t* parser = malloc(sizeof(flags_t));
    parser->count = 0;
    return parser;
}

void flags_destroy(flags_t* parser) {
    free(parser);
}

int flags_parse(flags_t* parser, int argc, char* argv[]) {
    for (int i = 1; i < argc - 1; i++) {
        for (int j = 0; j < parser->count; j++) {
            if (argv[i][0] == '-' && argv[i][1] != parser->flags[j].flag) {
                continue;
            }

            switch (parser->flags[j].type) {
                case FLAG_TYPE_INT:
                    *((int*)parser->flags[j].value) = atoi(argv[i + 1]);
                    break;
                case FLAG_TYPE_STRING:
                    strcpy((char*)parser->flags[j].value, argv[i + 1]);
                    break;
            }
        }
    }

    return 0;
}

int flags_int(flags_t* parser, void* value, char flag, int default_value) {
    if (parser->count >= 10) {
        return -1;
    }	

    parser->flags[parser->count].flag = flag;
    parser->flags[parser->count].type = FLAG_TYPE_INT;
    parser->flags[parser->count].value = value;
    parser->count++;

    // Set the default value
    *((int*)value) = default_value;

    return 0;
}

int flags_string(flags_t* parser, void* value, char flag, const char* default_value) {
    if (parser->count >= 10) {
        return -1;
    }
    
    parser->flags[parser->count].flag = flag;
    parser->flags[parser->count].type = FLAG_TYPE_STRING;
    parser->flags[parser->count].value = value;
    parser->count++;

    // Set the default value
    strcpy((char*)value, default_value);

    return 0;
}

