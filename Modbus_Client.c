#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "Modbus_Basic.h"
#include "Modbus_Function.h"

#define MODBUS_SERVER_IP "193.169.202.29"
#define MODBUS_SERVER_PORT 502
#define SLAVE_ID 1

int main() {
    modbus_t ctx = { .fd = -1, .transaction_id = 0 };
    char cmd[16];

    ctx.fd = modbus_connect(MODBUS_SERVER_IP, MODBUS_SERVER_PORT);
    if (ctx.fd < 0) { perror("Connect failed"); return -1; }
    printf("Connected to %s:%d\n", MODBUS_SERVER_IP, MODBUS_SERVER_PORT);

    while (1) {
        printf(LIGHT_BLUE "\nAvailable Commands:\n" NONE
        "[01] Read Coil, [03] Read Register, [05] Write Single Coil, [06] Write Single Register,"
        LIGHT_CYAN " [q] Quit\n" NONE);
        printf("Enter command: ");
        
        // 使用 %s 读取字符串，它会自动跳过之前的回车符
        if (scanf("%s", cmd) <= 0) break;
        if (strcmp(cmd, "q") == 0) {
            break;
        } else if (strcmp(cmd, "01") == 0 || strcmp(cmd, "03") == 0) {//01 读取线圈 03 读取保持寄存器
            uint16_t addr, qty;
            printf("Enter Address (0-65535): ");
            scanf("%hu", &addr);
            printf("Enter Quantity: ");
            scanf("%hu", &qty);
            if (strcmp(cmd, "01") == 0) {
                if (modbus_read_coils(&ctx, addr, qty) <= 0){
                    printf("Read failed.\n");
                    break;
                }
            }else if (strcmp(cmd, "03") == 0) {
                if (modbus_read_holding_registers(&ctx, addr, qty) <= 0) {
                    printf("Read failed.\n");
                    break;
                }
            }
        } else if (strcmp(cmd, "05") == 0 || strcmp(cmd, "06") == 0) {//05 写单个线圈 06 写单个保持寄存器
            uint16_t addr, value;
            printf("Enter Address(0-65535): ");
            scanf("%hu", &addr);
            printf("Enter Value: ");
            scanf("%hu", &value);
            if (strcmp(cmd, "05") == 0) {
                if (value > 1 || value < 0){
                    printf(RED "Error! Value must be 0 or 1.\n" NONE);
                    continue;
                }
                if (modbus_write_single(&ctx, addr, value, 0x05) != 0) {
                    printf("Write failed.\n");
                    break;
                }
            } else if (strcmp(cmd, "06") == 0) {
                if (modbus_write_single(&ctx, addr, value, 0x06) != 0) {
                    printf("Write failed.\n");
                    break;
                }
            }
        } else {
            printf("Unknown command: %s\n", cmd);
        }
    }
    close(ctx.fd);
    printf(LIGHT_CYAN "Client closed.\n" NONE);
    return 0;
}