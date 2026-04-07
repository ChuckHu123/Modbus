CC = gcc
CFLAGS = -Wall -g -I./include
TARGET_CLIENT = Modbus_Client
TARGET_SERVER = Modbus_Server
SRCS_DIR = src
CLIENT_SRCS = $(SRCS_DIR)/Modbus_Client.c $(SRCS_DIR)/Modbus_Function.c \
        $(SRCS_DIR)/Modbus_Basic.c $(SRCS_DIR)/Modbus_Protocol_Construction.c
SERVER_SRCS = $(SRCS_DIR)/Modbus_Server.c
LIBS_SERVER = -levent

all: $(TARGET_CLIENT) $(TARGET_SERVER)

$(TARGET_CLIENT): $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o ./build/$(TARGET_CLIENT) $(CLIENT_SRCS)

$(TARGET_SERVER): $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o ./build/$(TARGET_SERVER) $(SERVER_SRCS) $(LIBS_SERVER)

clean:
	rm -f ./build/$(TARGET_CLIENT) ./build/$(TARGET_SERVER)

.PHONY: all clean