#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "Modbus_Function.h"

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
    return fd;
}

// --- 校验 MBAP 报文头 ---
static int validate_mbap_header(const unsigned char *req, const unsigned char *res) {
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
    if (!validate_mbap_header(req, res)) {
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

void build_MBAP(modbus_t *ctx, unsigned char *req, uint16_t byte_len) {
    ctx->transaction_id++;
    req[0] = (ctx->transaction_id >> 8) & 0xFF;
    req[1] = ctx->transaction_id & 0xFF;
    req[2] = 0x00; req[3] = 0x00;//Modbus协议标识         
    req[4] = (byte_len >> 8) & 0xFF; req[5] = byte_len & 0xFF; //后续字节长度        
    req[6] = 0x01;// 单元标识符
}

void build_PDU_fc03(unsigned char *pdu, uint16_t addr, uint16_t qty){
    pdu[7] = 0x03;                          // 功能码
    pdu[8] = (addr >> 8) & 0xFF;           // 起始地址高字节
    pdu[9] = addr & 0xFF;                  // 起始地址低字节
    pdu[10] = (qty >> 8) & 0xFF;            // 数量高字节
    pdu[11] = qty & 0xFF;                   // 数量低字节
}

// --- 解析 03 功能码响应 ---
void parse_fc03(unsigned char *res, int byte_count, uint16_t addr) {
    for (int i = 0; i < byte_count / 2; i++) {
        int val = (res[9 + i*2] << 8) | res[10 + i*2];
        printf("Reg[%d]:%d  ", addr + i, val);
    }
    printf("\n");
}

// --- 03 功能码读取寄存器 ---
int modbus_read_holding_registers(modbus_t *ctx, uint16_t addr, uint16_t qty) {
    unsigned char req[12];
    unsigned char res[9+qty*2];// 计算响应长度MBAP 7+功能码1+字节计数1+数据位数（2*寄存器数量）
    build_MBAP(ctx, req, 0x06);
    build_PDU_fc03(req, addr, qty);

    if (send(ctx->fd, req, 12, 0) <= 0) {
        perror("Send failed.");
        return -1;
    }

    int len = modbus_receive(ctx, res, req);
    if (len > 0) {
        parse_fc03(res, res[8], addr);
    }
    return len;
}

// --- 06 功能码写单个寄存器 ---  !!!未完成
int modbus_write_single_register(modbus_t *ctx, uint16_t addr, uint16_t value) {
    unsigned char req[12];

    req[0] = (ctx->transaction_id >> 8) & 0xFF;
    req[1] = ctx->transaction_id & 0xFF;
    req[2] = 0x00; 
    req[3] = 0x00;
    req[4] = 0x00; 
    req[5] = 0x06;
    req[6] = 0x01;
    req[7] = 0x06;
    req[8] = (addr >> 8) & 0xFF;
    req[9] = addr & 0xFF;
    req[10] = (value >> 8) & 0xFF;
    req[11] = value & 0xFF;

    if (send(ctx->fd, req, 12, 0) <= 0) {
        return -1;
    }
    
    // 接收响应并校验
    unsigned char res[12];
    int len = modbus_receive(ctx, res, req);
    if (len > 0) {
        printf("Write confirmed. Address: %d, Value: %d\n", addr, value);
        return 0;
    }
    return -1;
}
