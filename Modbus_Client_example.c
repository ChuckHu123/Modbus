#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <modbus/modbus.h>
#include <errno.h>

// Modbus 服务器配置
#define MODBUS_SERVER_IP "193.169.202.29"  // HSL 仿真软件所在 IP
#define MODBUS_SERVER_PORT 502        // Modbus TCP 默认端口
#define SLAVE_ID 1                    // 从站 ID

int main() {
    modbus_t *ctx;//
    uint16_t tab_reg[32]; // 存储读取到的寄存器数据
    int rc;

    printf("=== Modbus 客户端启动 ===\n");
    printf("服务器地址：%s:%d ; 从站 ID: %d\n\n", MODBUS_SERVER_IP, MODBUS_SERVER_PORT, SLAVE_ID);

    ctx = modbus_new_tcp(MODBUS_SERVER_IP, MODBUS_SERVER_PORT);
    if (ctx == NULL) {
        fprintf(stderr, "无法创建 libmodbus 实例\n");
        return -1;
    }

    modbus_set_slave(ctx, SLAVE_ID);

    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "连接失败: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }
    printf("连接成功\n");

    // 读取保持寄存器 (起始地址 100, 读取 5 个)
    // 对应标准功能码 03
    rc = modbus_read_registers(ctx, 100, 5, tab_reg);
    if (rc == -1) {
        fprintf(stderr, "读取失败: %s\n", modbus_strerror(errno));
        modbus_close(ctx);
        modbus_free(ctx);
        return -1;
    }else{
        printf("读取成功\n");
        for (int i = 0; i < rc; i++) {
            printf("寄存器 [%d] = %d\n", 100+i, tab_reg[i]);
        }
    }

    printf("\n断开连接...\n");
    modbus_close(ctx);
    modbus_free(ctx);
    printf("程序结束。\n");

    return 0;
}