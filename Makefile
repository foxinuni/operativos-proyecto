CC = gcc
CFLAGS = -Icommon
SRC = source/main.c common/flags.c common/topics.c
OBJ = main.o
TARGET = program

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)