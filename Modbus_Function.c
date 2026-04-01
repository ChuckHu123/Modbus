#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "Modbus_Function.h"

// ---  建立连接 ---
int modbus_connect(const char *ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

// --- 修改后的 03 功能码读取函数，支持指定地址和数量 ---
int modbus_read_holding_registers(modbus_t *ctx, uint16_t addr, uint16_t qty) {
    unsigned char req[12];
    ctx->transaction_id++;

    req[0] = (ctx->transaction_id >> 8) & 0xFF;
    req[1] = ctx->transaction_id & 0xFF;
    req[2] = 0x00; req[3] = 0x00;           
    req[4] = 0x00; req[5] = 0x06;           
    req[6] = 0x01;                          
    req[7] = 0x03;//功能码
    req[8] = (addr >> 8) & 0xFF;
    req[9] = addr & 0xFF;
    req[10] = (qty >> 8) & 0xFF;
    req[11] = qty & 0xFF;

    return send(ctx->fd, req, 12, 0);
}

// --- 预留：将来你可以加个 06 功能码（写单个寄存器） ---
/*
int modbus_write_single_register(modbus_t *ctx, uint16_t addr, uint16_t value) {
    // 构建 12 字节报文，FC=06...
}
*/

// 统一的接收函数，只负责提取通用的 Header
int modbus_receive(modbus_t *ctx, unsigned char *res) {
    int len = recv(ctx->fd, res, BUFFER_SIZE, 0);
    if (len <= 0) return len;

    // 检查是否是异常报文 (功能码最高位为 1)
    if (res[7] & 0x80) {
        printf("Modbus Exception! Code: %02X\n", res[8]);
        return -1;
    }
    return len;
}

// 专门解析 03 的逻辑
void parse_fc03(unsigned char *res, int byte_count, uint16_t addr) {
    for (int i = 0; i < byte_count / 2; i++) {
        int val = (res[9 + i*2] << 8) | res[10 + i*2];
        printf("Reg[%d]: %d ", addr + i, val);
    }
    printf("\n");
}