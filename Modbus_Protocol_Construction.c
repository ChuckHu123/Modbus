#include "Modbus_Basic.h"
#include "Modbus_Protocol_Construction.h"


void build_MBAP(modbus_t *ctx, unsigned char *req, uint16_t byte_len) {
    ctx->transaction_id++;
    req[0] = (ctx->transaction_id >> 8) & 0xFF;
    req[1] = ctx->transaction_id & 0xFF;
    req[2] = 0x00; req[3] = 0x00;//Modbus协议标识         
    req[4] = (byte_len >> 8) & 0xFF; req[5] = byte_len & 0xFF; //后续字节长度        
    req[6] = 0x01;// 单元标识符
}

void build_PDU_fc01(unsigned char *pdu, uint16_t addr, uint16_t qty){
    pdu[7] = 0x01;                          // 功能码
    pdu[8] = (addr >> 8) & 0xFF;           // 起始地址高字节
    pdu[9] = addr & 0xFF;                  // 起始地址低字节
    pdu[10] = (qty >> 8) & 0xFF;            // 数量高字节
    pdu[11] = qty & 0xFF;                   // 数量低字节
}

void build_PDU_fc03(unsigned char *pdu, uint16_t addr, uint16_t qty){
    pdu[7] = 0x03;                          // 功能码
    pdu[8] = (addr >> 8) & 0xFF;           // 起始地址高字节
    pdu[9] = addr & 0xFF;                  // 起始地址低字节
    pdu[10] = (qty >> 8) & 0xFF;            // 数量高字节
    pdu[11] = qty & 0xFF;                   // 数量低字节
}

void build_PDU_fc05(unsigned char *pdu, uint16_t addr, uint16_t value){
    pdu[7] = 0x05;
    pdu[8] = (addr >> 8) & 0xFF;
    pdu[9] = addr & 0xFF;
    // ON=0xFF00, OFF=0x0000
    pdu[10] = (value != 0) ? 0xFF : 0x00;
    pdu[11] = 0x00;  // 固定为 0x00
}

void build_PDU_fc06(unsigned char *pdu, uint16_t addr, uint16_t value){
    pdu[7] = 0x06;
    pdu[8] = (addr >> 8) & 0xFF;
    pdu[9] = addr & 0xFF;
    pdu[10] = (value >> 8) & 0xFF;
    pdu[11] = value & 0xFF;
}