#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "Modbus_Function.h"

static void build_MBAP(modbus_t *ctx, unsigned char *req, uint16_t byte_len) {
    ctx->transaction_id++;
    req[0] = (ctx->transaction_id >> 8) & 0xFF;
    req[1] = ctx->transaction_id & 0xFF;
    req[2] = 0x00; req[3] = 0x00;//Modbus协议标识         
    req[4] = (byte_len >> 8) & 0xFF; req[5] = byte_len & 0xFF; //后续字节长度        
    req[6] = 0x01;// 单元标识符
}

static void build_PDU_fc03(unsigned char *pdu, uint16_t addr, uint16_t qty){
    pdu[7] = 0x03;                          // 功能码
    pdu[8] = (addr >> 8) & 0xFF;           // 起始地址高字节
    pdu[9] = addr & 0xFF;                  // 起始地址低字节
    pdu[10] = (qty >> 8) & 0xFF;            // 数量高字节
    pdu[11] = qty & 0xFF;                   // 数量低字节
}

// --- 解析 03 功能码响应 ---
static void parse_fc03(unsigned char *res, int byte_count, uint16_t addr) {
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

static void build_PDU_fc06(unsigned char *pdu, uint16_t addr, uint16_t value){
    pdu[7] = 0x06;
    pdu[8] = (addr >> 8) & 0xFF;
    pdu[9] = addr & 0xFF;
    pdu[10] = (value >> 8) & 0xFF;
    pdu[11] = value & 0xFF;
}

// --- 06 功能码写单个寄存器 ---
int modbus_write_single_register(modbus_t *ctx, uint16_t addr, uint16_t value) {
    unsigned char req[12];
    unsigned char res[12];
    build_MBAP(ctx, req, 0x06);
    build_PDU_fc06(req, addr, value);

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

    printf("Write confirmed. Address: %d, Value: %d\n", addr, value);
    return 0;
}
