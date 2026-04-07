#ifndef MODBUS_FUNCTION_H
#define MODBUS_FUNCTION_H

#include "Modbus_Basic.h"
#include "Modbus_Protocol_Construction.h"

// --- Modbus 功能函数 ---
int modbus_read_coils(modbus_t *ctx, uint16_t addr, uint16_t qty);
int modbus_read_holding_registers(modbus_t *ctx, uint16_t addr, uint16_t qty);
int modbus_write_single(modbus_t *ctx, uint16_t addr, uint16_t value, uint16_t function_code);
int modbus_write_multiple(modbus_t *ctx, uint16_t addr, uint16_t qty, uint16_t function_code);

#endif