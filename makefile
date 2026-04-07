CC = gcc
CFLAGS = -Wall -g -I./include
TARGET = Modbus_Client
SRCS_DIR = src
SRCS = $(SRCS_DIR)/Modbus_Client.c $(SRCS_DIR)/Modbus_Function.c \
		$(SRCS_DIR)/Modbus_Basic.c $(SRCS_DIR)/Modbus_Protocol_Construction.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o ./build/$(TARGET) $(SRCS)

clean:
	rm -f ./build/$(TARGET)

.PHONY: all clean