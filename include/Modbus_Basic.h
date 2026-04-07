#ifndef MODBUS_BASIC_H
#define MODBUS_BASIC_H

#include <stdint.h>

// --- 定义颜色 ---
#define NONE "\033[m"
#define RED "\033[0;32;31m"
#define LIGHT_BLUE "\033[1;34m"
#define LIGHT_CYAN "\033[1;36m"

#define BUFFER_SIZE 1024

// --- 结构体定义，方便管理 ---
typedef struct {
    int fd;
    uint16_t transaction_id;
} modbus_t;

int modbus_connect(const char *ip, int port);
int modbus_receive(modbus_t *ctx, unsigned char *res, const unsigned char *req);

#endif