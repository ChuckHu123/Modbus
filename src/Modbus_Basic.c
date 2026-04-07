#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../include/Modbus_Basic.h"

// --- 建立连接 ---
int modbus_connect(const char *ip, int port) {//返回文件描述符
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

    // 设置接收超时为 5 秒
    struct timeval timeout;
    timeout.tv_sec = 5;   // 5 秒
    timeout.tv_usec = 0;
    
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        close(fd);
        return -1;
    }
    
    return fd;
}

// --- 校验 MBAP 报文头 ---
static int validate_mbap_header(const unsigned char *req, const unsigned char *res, int received_len) {
    // 1. 检查事务 ID 是否匹配
    if (req[0] != res[0] || req[1] != res[1]) {
        printf("Transaction ID mismatch! req=%02X%02X, res=%02X%02X\n", 
               req[0], req[1], res[0], res[1]);
        return 0;
    }

    // 2. 检查协议 ID 是否为 0x0000 (Modbus)
    if (res[2] != 0x00 || res[3] != 0x00) {
        printf("Invalid Protocol ID! Expected 0x0000, got %02X%02X\n", 
               res[2], res[3]);
        return 0;
    }

    // 3. 检查MBAP的长度字段是否匹配
    uint16_t declared_length = (res[4]<<8) | res[5];
    if (declared_length != (received_len - 6)) {
        printf("Response length mismatch! Expected %d, got %d\n", declared_length, (received_len - 6));
        return 0;
    }
    return 1;
}

// --- 统一的接收函数，包含 MBAP 校验 返回数据长度 ---
int modbus_receive(modbus_t *ctx, unsigned char *res, const unsigned char *req) {
    int len = recv(ctx->fd, res, BUFFER_SIZE, 0);
    if (len <= 0) {
        printf("Receive failed or connection closed.\n");
        return len;
    }

    // 确保至少有 7 字节的 MBAP 头 + 单元标识符
    if (len < 7) {
        printf("Response too short for MBAP header.\n");
        return -1;
    }

    // 验证 MBAP 报文头
    if (!validate_mbap_header(req, res, len)) {
        printf("MBAP Header validation failed!\n");
        return -1;
    }

    // 检查是否是异常报文 (功能码最高位为 1)
    if (res[7] & 0x80) {
        printf("Modbus Exception! Code: %02X\n", res[8]);
        return -1;
    }
    
    return len;
}