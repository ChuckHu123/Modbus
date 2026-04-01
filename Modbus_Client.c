#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
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
        printf("\nAvailable Commands: [03] Read, [q] Quit\n");
        printf("Enter command: ");
        unsigned char res[BUFFER_SIZE];
        
        // 使用 %s 读取字符串，它会自动跳过之前的回车符
        if (scanf("%s", cmd) <= 0) break;

        if (strcmp(cmd, "q") == 0) {
            break;
        } else if (strcmp(cmd, "03") == 0) {
            uint16_t addr, qty;
            printf("Enter Address (0-65535) and Quantity: ");
            scanf("%hu %hu", &addr, &qty); // %hu 用于读取无符号短整型

            if (modbus_read_holding_registers(&ctx, addr, qty) > 0) {
                int len = modbus_receive(&ctx, res);
                if (len > 0) {
                    parse_fc03(res, res[8], addr);
                }
            } else {
                printf("Send failed.\n");
                break;
            }
        } else if (strcmp(cmd, "06") == 0) {
            // 这里以后放 06 功能码的逻辑
        } else {
            printf("Unknown command: %s\n", cmd);
        }
    }

    close(ctx.fd);
    printf("Client closed.\n");
    return 0;
}