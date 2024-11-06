#ifndef SC_PACKET
#define SC_PACKET
#include <string.h>

// The following structure represents a singular flag
// It contains the flag character, the type of the flag, the default value, and the value
// When parsing, the value will be updated to the parsed value
typedef struct {
    char flag;
    enum {
        FLAG_TYPE_INT,
        FLAG_TYPE_STRING,
    } type;
    void* value;
} flag_t;

// The following structure represents a flag parser.
// It contains an array of flags, and the count of flags.
// I probably should have used a linked list, but I don't want to abuse the heap
// more than I need to. So a hard limit of 10 flags is imposed.
typedef struct {
    flag_t flags[10];
    short count;
} flags_t;

// Creates a new flag parser
// Make sure to call flags_destroy when you're done with it
flags_t* flags_create();

// Destroys a flag parser (frees the flags but not the values parsed into them)
void flags_destroy(flags_t* parser);

// Parses the command line arguments, make sure to call this after adding all the flags
int flags_parse(flags_t* parser, int argc, char* argv[]);

// Adds an integer flag to the parser
int flags_int(flags_t* parser, void* value, char flag, int default_value);

// Adds a string flag to the parser
int flags_string(flags_t* parser, void* value, char flag, const char* default_value);

#endif