#include <stdio.h>
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

void build_PDU_fc0f(unsigned char *pdu, uint16_t addr, uint16_t qty){
    pdu[7] = 0x0F;
    pdu[8] = (addr >> 8) & 0xFF;
    pdu[9] = addr & 0xFF;
    pdu[10] = (qty >> 8) & 0xFF;
    pdu[11] = qty & 0xFF;
    // 计算需要的字节数（每8个线圈占1个字节，向上取整）
    uint8_t byte_count = (qty + 7) / 8;
    pdu[12] = byte_count;

    // 初始化数据区域为0
    for (int i = 0; i < byte_count; i++) {
        pdu[13 + i] = 0x00;
    }

    for (int i = 0; i < qty; i++){
        printf("Enter the value of coil [%d] (0 or 1):", addr + i);
        uint16_t value;
        scanf("%hu", &value);
        if (value == 0 || value == 1){
            // 将第i个线圈的状态设置到对应的字节和位
            int byte_index = i / 8;      // 确定在第几个字节
            int bit_index = i % 8;       // 确定在该字节的第几位
            if (value != 0) {
                pdu[13 + byte_index] |= (1 << bit_index);  // 设置对应位为1
            }
        }else{
            printf("Invalid value. Please enter 0 or 1.\n");
            i--;
        }
    }
}

void build_PDU_fc10(unsigned char *pdu, uint16_t addr, uint16_t qty){
    pdu[7] = 0x10;
    pdu[8] = (addr >> 8) & 0xFF;
    pdu[9] = addr & 0xFF;
    pdu[10] = (qty >> 8) & 0xFF;
    pdu[11] = qty & 0xFF;
    pdu[12] = qty * 2;
    for (int i = 0; i < qty; i++){
        printf("Enter the value of Register [%d]:",addr + i);
        uint16_t value; 
        scanf("%hu", &value);
        pdu[13 + 2*i] = (value >> 8) & 0xFF;
        pdu[14 + 2*i] = value & 0xFF;
    }
}
