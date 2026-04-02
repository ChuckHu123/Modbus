#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "Modbus_Function.h"

// --- 解析 01 功能码响应 ---
static void parse_fc01(unsigned char *res, int byte_count, uint16_t addr, uint16_t qty) {
    printf("Raw data (hex): ");
    // 打印原始数据的 16 进制
    for (int i = 0; i < byte_count; i++) {
        printf("%02X ", res[9 + i]);
    }
    printf("\n");

    // 逐个线圈读取数据
    for (int i =0; i < qty; i++) {
         if(i % 10 == 0 && i != 0)printf("\n");
        printf("Coil[%d]: %d  ", addr + i, (res[9+ i/8] >> (i % 8)) & 0x01);
    }
    printf("\n");
}

// --- 01 功能码读取线圈 ---
int modbus_read_coils(modbus_t *ctx, uint16_t addr, uint16_t qty) {
    unsigned char req[12];
    unsigned char res[9+(qty/8+1)];// 计算响应长度MBAP 7+功能码1+字节计数1+数据位数（1字节=8位），加一因为除法会截掉小数
    build_MBAP(ctx, req, 0x06);
    build_PDU_fc01(req, addr, qty);

    if (send(ctx->fd, req, 12, 0) <= 0) {
        perror("Send failed.");
        return -1;
    }

    int len = modbus_receive(ctx, res, req);
    if (len > 0) {
        parse_fc01(res, res[8], addr, qty);
    }
    return len;
}
// --- 解析 03 功能码响应 ---
static void parse_fc03(unsigned char *res, int byte_count, uint16_t addr) {
    for (int i = 0; i < byte_count / 2; i++) {
        int val = (res[9 + i*2] << 8) | res[10 + i*2];
        if(i % 10 == 0 && i != 0)printf("\n");
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

// --- 05 功能码写单个线圈 06 功能码写单个寄存器 ---
int modbus_write_single(modbus_t *ctx, uint16_t addr, uint16_t value, uint16_t function_code) {
    unsigned char req[12];
    unsigned char res[12];
    build_MBAP(ctx, req, 0x06);

    if (function_code == 0x05) {
        build_PDU_fc05(req, addr, value);
    } else if (function_code == 0x06) {
        build_PDU_fc06(req, addr, value);
    } else {
        return -1;
    }

    if (send(ctx->fd, req, 12, 0) <= 0) {
        perror("Send failed.");
        return -1;
    }

    if (modbus_receive(ctx, res, req) <= 0) {
        if (memcmp(res, req, 12) != 0) {
            perror("Response mismatch!\n");
            return -1;
        }
    }
    printf(LIGHT_BLUE "Write confirmed. Address: %d, Value: %d\n" NONE, addr, value);
    return 0;
}
