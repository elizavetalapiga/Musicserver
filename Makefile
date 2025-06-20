# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lsqlite3

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Object files
CLIENT_OBJS = $(OBJ_DIR)/serverclient.o $(OBJ_DIR)/recieve_handler.o $(OBJ_DIR)/network_utils.o $(OBJ_DIR)/login_client.o $(OBJ_DIR)/tag_handler.o $(OBJ_DIR)/cache_handler.o $(OBJ_DIR)/disk_space.o
SERVER_OBJS = $(OBJ_DIR)/simpleserver.o $(OBJ_DIR)/request_handler.o $(OBJ_DIR)/network_utils.o $(OBJ_DIR)/login.o $(OBJ_DIR)/tag_handler.o $(OBJ_DIR)/db_handler.o $(OBJ_DIR)/disk_space.o

# Default build: build both server and client
all: $(BIN_DIR)/serverclient $(BIN_DIR)/simpleserver

# Build client
$(BIN_DIR)/serverclient: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Build server
$(BIN_DIR)/simpleserver: $(SERVER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile object files
$(OBJ_DIR)/serverclient.o: $(SRC_DIR)/serverclient.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/simpleserver.o: $(SRC_DIR)/simpleserver.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/recieve_handler.o: $(SRC_DIR)/recieve_handler.c include/recieve_handler.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/request_handler.o: $(SRC_DIR)/request_handler.c include/request_handler.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/network_utils.o: $(SRC_DIR)/network_utils.c include/network_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/login_client.o: $(SRC_DIR)/login_client.c include/login_client.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/login.o: $(SRC_DIR)/login.c include/login.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/tag_handler.o: $(SRC_DIR)/tag_handler.c include/tag_handler.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/db_handler.o: $(SRC_DIR)/db_handler.c include/db_handler.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/cache_handler.o: $(SRC_DIR)/cache_handler.c include/cache_handler.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/disk_space.o: $(SRC_DIR)/disk_space.c include/disk_space.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/*
