CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lm
COMMON = src/commons.c src/flags.c

central: src/central.c $(COMMON)
	$(CC) $(CFLAGS) -o central src/central.c $(COMMON) $(LDFLAGS) 

publisher: src/publisher.c $(COMMON)
	$(CC) $(CFLAGS) -o publisher src/publisher.c $(COMMON) $(LDFLAGS)

subscriber: src/subscriber.c $(COMMON)
	$(CC) $(CFLAGS) -o subscriber src/subscriber.c $(COMMON) $(LDFLAGS)

clean:
	rm -f central publisher subscriber