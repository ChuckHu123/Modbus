#ifndef MODBUS_FUNCTION_H
#define MODBUS_FUNCTION_H

#include <stdint.h>

#define BUFFER_SIZE 1024

// --- 1. 结构体定义，方便管理 ---
typedef struct {
    int fd;
    uint16_t transaction_id;
} modbus_t;

int modbus_connect(const char *ip, int port);
int modbus_read_holding_registers(modbus_t *ctx, uint16_t addr, uint16_t qty);
int modbus_write_single_register(modbus_t *ctx, uint16_t addr, uint16_t value);
int modbus_receive(modbus_t *ctx, unsigned char *res, const unsigned char *req);
void parse_fc03(unsigned char *res, int byte_count, uint16_t addr);

#endif