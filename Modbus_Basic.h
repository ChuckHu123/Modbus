#ifndef MODBUS_BASIC_H
#define MODBUS_BASIC_H

#include <stdint.h>


#define BUFFER_SIZE 1024

// --- 结构体定义，方便管理 ---
typedef struct {
    int fd;
    uint16_t transaction_id;
} modbus_t;

int modbus_connect(const char *ip, int port);
int modbus_receive(modbus_t *ctx, unsigned char *res, const unsigned char *req);

#endif