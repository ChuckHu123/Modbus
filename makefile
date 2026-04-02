CC = gcc
CFLAGS = -Wall -g
TARGET = Modbus_Client
SRCS = Modbus_Client.c Modbus_Function.c Modbus_Basic.c Modbus_Protocol_Construction.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o ./build/$(TARGET) $(SRCS)

clean:
	rm -f ./build/$(TARGET)

.PHONY: all clean