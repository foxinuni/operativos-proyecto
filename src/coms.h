#ifndef SC_COMS_H
#define SC_COMS_H

// subscribers
void sub_register(int fd, int pid, char* interests);
void sub_unregister(int fd, int pid);

// publishers
void pub_register(int fd, int pid);
void pub_unregister(int fd, int pid);

#endif
