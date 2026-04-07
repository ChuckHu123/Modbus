#ifndef MODBUS_PROTOCOL_CONSTRUCTION_H
#define MODBUS_PROTOCOL_CONSTRUCTION_H

void build_MBAP(modbus_t *ctx, unsigned char *req, uint16_t byte_len);
void build_PDU_fc01(unsigned char *pdu, uint16_t addr, uint16_t qty);
void build_PDU_fc03(unsigned char *pdu, uint16_t addr, uint16_t qty);
void build_PDU_fc05(unsigned char *pdu, uint16_t addr, uint16_t value);
void build_PDU_fc06(unsigned char *pdu, uint16_t addr, uint16_t value);
void build_PDU_fc10(unsigned char *pdu, uint16_t addr, uint16_t qty);

#endif
