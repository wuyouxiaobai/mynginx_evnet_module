CC = g++
CFLAGS = -Wall -std=c++11
SRC_CLIENT = test.cpp
OBJ_CLIENT = test.o
TARGET_CLIENT = client_test

all: $(TARGET_CLIENT)

$(TARGET_CLIENT): $(OBJ_CLIENT)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_CLIENT): $(SRC_CLIENT)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_CLIENT) $(TARGET_CLIENT)