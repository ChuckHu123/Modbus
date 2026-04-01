#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MODBUS_SERVER_IP "193.169.202.29"
#define MODBUS_SERVER_PORT 502
#define SLAVE_ID 1
#define BUFFER_SIZE 1024

// --- 1. 结构体定义，方便管理 ---
typedef struct {
    int fd;
    uint16_t transaction_id;
} modbus_t;

// --- 2. 建立连接 ---
int modbus_connect(const char *ip, int port) {
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

// --- 3. 构建并发送 03 请求 ---
// 这里我们手动计算 Length，为以后支持其他功能码打基础
int modbus_read_registers(modbus_t *ctx, uint16_t addr, uint16_t quantity) {//03功能码
    unsigned char req[12];
    ctx->transaction_id++;

    // MBAP Header
    req[0] = (ctx->transaction_id >> 8) & 0xFF;
    req[1] = ctx->transaction_id & 0xFF;
    req[2] = 0x00; req[3] = 0x00;           // 协议标识，MODBUS固定为0
    req[4] = 0x00; req[5] = 0x06;           // 后续单元长度
    req[6] = 0x01;                          // 从站ID
    
    // PDU
    req[7] = 0x03;                          // 功能码
    req[8] = (addr >> 8) & 0xFF;            // 起始地址高字节
    req[9] = addr & 0xFF;                   // 起始地址低字节
    req[10] = (quantity >> 8) & 0xFF;       // 数量高字节
    req[11] = quantity & 0xFF;              // 数量低字节

    return send(ctx->fd, req, 12, 0);
}

// --- 4. 接收并解析响应 ---
int modbus_receive_and_parse(modbus_t *ctx) {
    unsigned char res[BUFFER_SIZE];
    int len = recv(ctx->fd, res, sizeof(res), 0);
    
    if (len <= 0) return -1;

    // 简单校验响应码是否匹配请求
    if (res[7] == 0x03) {
        int byte_count = res[8];// 数据字节数
        printf("Received %d bytes data. Values: ", byte_count);
        for (int i = 0; i < byte_count / 2; i++) {//byte_count / 2 表示寄存器个数
            // Modbus 是大端，高字节在前，需要组合字节
            int val = (res[9 + i*2] << 8) | res[10 + i*2];
            printf("[%d] ", val);
        }
        printf("\n");
        return 0;
    } else if (res[7] & 0x80) {
        printf("Modbus Exception! Error Code: %02X\n", res[8]);
    }
    return -1;
}

int main() {
    modbus_t ctx = { .fd = -1, .transaction_id = 0 };

    /*
    char ip[16];
    int port;
    printf("Enter Server IP: ");
    scanf("%15s", ip);
    printf("Enter Port: ");
    scanf("%d", &port);
    */

    ctx.fd = modbus_connect(MODBUS_SERVER_IP, MODBUS_SERVER_PORT);
    if (ctx.fd < 0) {
        perror("Connect failed");
        return -1;
    }
    printf("Connected to %s:%d\n", MODBUS_SERVER_IP, MODBUS_SERVER_PORT);

    // 问答循环
    while (1) {
        printf("Press Enter to read Register 0 (or 'q' to quit)...");
        //getchar(); // 吃掉之前的回车
        if (getchar() == 'q') break;

        // 发送 03 请求：读取地址 0，数量 1
        if (modbus_read_registers(&ctx, 0, 1) < 0) {
            printf("Send failed.\n");
            break;
        }

        // 接收响应
        if (modbus_receive_and_parse(&ctx) < 0) {
            printf("Receive failed or Server disconnected.\n");
            break;
        }
    }

    close(ctx.fd);
    printf("Client closed.\n");
    return 0;
}