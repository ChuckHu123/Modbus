//所以运行的时候要加sudo，端口 502 是 Modbus TCP 的标准端口，但它是特权端口（小于 1024），需要 root 权限才能绑定
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#define MODBUS_SERVER_PORT 502
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

// 模拟的寄存器存储空间
#define MAX_REGISTERS 1000
uint16_t holding_registers[MAX_REGISTERS] = {0};
uint8_t coils[MAX_REGISTERS] = {0};

// 客户端数据结构
typedef struct {
    struct bufferevent *bev;
    unsigned char recv_buffer[BUFFER_SIZE];  // 接收缓冲区
    size_t recv_len;                          // 当前缓冲区中的数据长度
} client_data_t;

void build_MBAP_header(unsigned char *buffer, uint16_t transaction_id, uint16_t length) {
    buffer[0] = (transaction_id >> 8) & 0xFF;
    buffer[1] = transaction_id & 0xFF;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = (length >> 8) & 0xFF;
    buffer[5] = length & 0xFF;
    buffer[6] = 0x01;
}

int build_exception_response(unsigned char *request, uint8_t exception_code, unsigned char *response) {
    uint16_t transaction_id = (request[0] << 8) | request[1];
    build_MBAP_header(response, transaction_id, 3);
    response[7] = request[7] | 0x80;
    response[8] = exception_code;
    return 9;
}

int handle_read_coils(unsigned char *request, size_t request_len, unsigned char *response) {
    if (request_len < 12){
        printf("Invalid request: too short\n");
        return 0;
    }
    int16_t start_addr = (request[8] << 8) | request[9];
    uint16_t quantity = (request[10] << 8) | request[11];
    // 检查地址范围
    if (start_addr < 0 || quantity == 0 || start_addr + quantity > MAX_REGISTERS) {
        return build_exception_response(request, 0x02, response);
    }
    uint8_t byte_count = (quantity + 7) / 8;//计算字节数

    uint16_t transaction_id = (request[0] << 8) | request[1];
    build_MBAP_header(response, transaction_id, 3 + byte_count);
    response[7] = 0x01; // Function code
    response[8] = byte_count; // Byte count

    memset(&response[9], 0, byte_count);
    for (int i = 0; i < quantity; i++) {
        if (coils[start_addr + i]) {
            int byte_index = i / 8;
            int bit_index = i % 8;
            response[9 + byte_index] |= (1 << bit_index);
        }
    }
    return 9 + byte_count;
}

int handle_read_holding_registers(unsigned char *request, size_t request_len, unsigned char *response) {
    if (request_len < 12){
        printf("Invalid request: too short\n");
        return 0;
    }
    int16_t start_addr = (request[8] << 8) | request[9];
    uint16_t quantity = (request[10] << 8) | request[11];
    if (start_addr < 0 || quantity == 0 || start_addr + quantity > MAX_REGISTERS) {
        return build_exception_response(request, 0x02, response);
    }
    uint16_t transaction_id = (request[0] << 8) | request[1];
    build_MBAP_header(response, transaction_id, 3 + 2 * quantity);
    response[7] = 0x03;
    response[8] = 2 * quantity;

    memset(&response[9], 0, 2 * quantity);
    for(int i=0; i<quantity;i++){
        response[9 + i * 2] = (holding_registers[start_addr + i] >> 8) & 0xFF;
        response[10 + i * 2] = holding_registers[start_addr + i] & 0xFF;
    }
    return 9 + 2 * quantity;
}

int handle_write_single_coil(unsigned char *request, size_t request_len, unsigned char *response) {
    if (request_len < 12){
        printf("Invalid request: too short\n");
        return 0;
    }
    int16_t addr = (request[8] << 8) | request[9];
    uint16_t value = (request[10] << 8) | request[11];

    if (addr < 0 || addr >= MAX_REGISTERS) {
        return build_exception_response(request, 0x02, response);
    }

    if (value != 0x0000 && value != 0xFF00) {
        return build_exception_response(request, 0x03, response);
    }
    coils[addr] = (value == 0xFF00)? 1:0;
    printf("Write coil[%d] = %d\n", addr, coils[addr]);

    memcpy(response, request, 12);
    return 12;
}

int handle_write_single_register(unsigned char *request, size_t request_len, unsigned char *response) {
    if (request_len < 12){
        printf("Invalid request: too short\n");
        return 0;
    }
    int16_t addr = (request[8] << 8) | request[9];
    uint16_t value = (request[10] << 8) | request[11];

    if (addr < 0 || addr >= MAX_REGISTERS) {
        return build_exception_response(request, 0x02, response);
    }

    holding_registers[addr] = value;
    printf("Write register[%d] = %d\n", addr, holding_registers[addr]);
    memcpy(response, request, 12);
    return 12;
}

int handle_write_multiple_coils(unsigned char *request, size_t request_len, unsigned char *response) {
    uint16_t start_addr = (request[8] << 8) | request[9];
    uint16_t quantity = (request[10] << 8) | request[11];
    uint8_t byte_count = request[12];
    // 检查参数有效性
    if (start_addr + quantity > MAX_REGISTERS || byte_count != (quantity + 7) / 8) {
        return build_exception_response(request, 0x02, response);
    }
    if (request_len < 13+byte_count){
        printf("Invalid request: too short\n");
        return 0;
    }

    for(int i=0;i<quantity;i++){
        int byte_index = i / 8;
        int bit_index = i % 8;
        coils[start_addr + i] = (request[13 + byte_index] >> bit_index) & 0x01;
    }
    printf("Write %d coils starting at %d\n", quantity, start_addr);

    uint16_t transaction_id = (request[0] << 8) | request[1];
    build_MBAP_header(response, transaction_id, 6);
    response[7] = 0x0F;
    response[8] = (start_addr >> 8) & 0xFF;
    response[9] = start_addr & 0xFF;
    response[10] = (quantity >> 8) & 0xFF;
    response[11] = quantity & 0xFF;
    return 12;
}

int handle_write_multiple_registers(unsigned char *request, size_t request_len, unsigned char *response) {
    uint16_t start_addr = (request[8] << 8) | request[9];
    uint16_t quantity = (request[10] << 8) | request[11];
    uint8_t byte_count = request[12];
    if (start_addr + quantity > MAX_REGISTERS || byte_count != 2 * quantity) {
        return build_exception_response(request, 0x02, response);
    }
    if (request_len < 13+byte_count){
        printf("Invalid request: too short\n");
        return 0;
    }
    for(int i=0;i<quantity;i++){
        holding_registers[start_addr + i] = (request[13 + i * 2] << 8) | request[14 + i * 2];
    }
    printf("Write %d registers starting at %d\n", quantity, start_addr);

    uint16_t transaction_id = (request[0] << 8) | request[1];
    build_MBAP_header(response, transaction_id, 6);
    response[7] = 0x10;
    response[8] = (start_addr >> 8) & 0xFF;
    response[9] = start_addr & 0xFF;
    response[10] = (quantity >> 8) & 0xFF;
    response[11] = quantity & 0xFF;
    return 12;
}

// 处理 Modbus 请求并构建响应，返回响应长度
int process_modbus_request(unsigned char *req, size_t request_len, unsigned char *response) {
    if (request_len < 8) {
        printf("Invalid request: too short\n");
        return 0;
    }

    if (req[2] != 0x00 || req[3] != 0x00){ //检查协议标识符
        printf("Invalid request: protocol ID mismatch\n");
        return 0;
    }

    if ((req[4]<<8 | req[5]) != request_len - 6){ //检查数据长度
        printf("Invalid request: length mismatch\n");
        return 0;
    }

    uint8_t function_code = req[7];
    switch (function_code) {
        case 0x01: // 读线圈
            return handle_read_coils(req, request_len, response);
        case 0x03: // 读保持寄存器
            return handle_read_holding_registers(req, request_len, response);
        case 0x05: // 写单个线圈
            return handle_write_single_coil(req, request_len, response);
        case 0x06: // 写单个寄存器
            return handle_write_single_register(req, request_len, response);
        case 0x0F: // 写多个线圈
            return handle_write_multiple_coils(req, request_len, response);
        case 0x10: // 写多个寄存器
            return handle_write_multiple_registers(req, request_len, response);
        default:
            printf("Unsupported function code: 0x%02X\n", function_code);
            return build_exception_response(req, 0x01, response); // 非法功能码
        }
}

// 【回调1】当客户端发送数据时，libevent 自动调用此函数
void read_cb(struct bufferevent *bev, void *ctx) {
    client_data_t *client = (client_data_t *)ctx;
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t available = evbuffer_get_length(input);
    
    if (available == 0) return;

    // 防止缓冲区溢出
    if (client->recv_len >= BUFFER_SIZE) {
        fprintf(stderr, "Buffer overflow! Resetting buffer.\n");
        client->recv_len = 0;
    }

    size_t nread = evbuffer_remove(input, 
            client->recv_buffer + client->recv_len, 
            sizeof(client->recv_buffer) - client->recv_len);
    client->recv_len += nread;
    
    printf("Received %zu bytes, total buffer: %zu bytes\n", nread, client->recv_len);
    
    while(client->recv_len >= 7){
        uint16_t mbap_len = (client->recv_buffer[4]<<8) | client->recv_buffer[5];//计算 MBAP 中的长度字段
        size_t total_len=6+mbap_len;//完整数据长度

        // 检查 MBAP 长度字段是否合理
        if (mbap_len > 512) {
            printf("Invalid MBAP length: %u, closing connection\n", mbap_len);
            bufferevent_free(bev);
            return;
        }
        //检查完整帧是否超过缓冲区大小
        if (total_len > BUFFER_SIZE) {
            printf("Frame too large: %zu bytes, closing connection\n", total_len);
            bufferevent_free(bev);
            return;
        }
        
        if(client->recv_len<total_len){
            printf("Incomplete frame, waiting for more data\n");
            break;
        }

        unsigned char response[BUFFER_SIZE];
        int response_len = process_modbus_request(client->recv_buffer, total_len, response);// 处理 Modbus 请求并构建响应
        if (response_len > 0) {
            bufferevent_write(bev, response, response_len);
            printf("Send :");
            for (int i = 0; i < response_len; i++) {
                printf(" %02X", response[i]);
            }
            printf("\n");
        }else {
            printf("Invalid Modbus request received, closing connection\n");
            bufferevent_free(bev);
            return;
        }

        // 从缓冲区中移除已处理的帧，把后面没处理的盖到前面去
        size_t remaining = client->recv_len - total_len;
        if (remaining > 0) {
            memmove(client->recv_buffer, 
                    client->recv_buffer + total_len, 
                    remaining);
        }
        client->recv_len = remaining;

        printf("Processed frame (%zu bytes), remaining in buffer: %zu bytes\n", 
               total_len, client->recv_len);
    }
    
}

// 【回调2】当连接发生错误或断开时，libevent 自动调用此函数
void event_cb(struct bufferevent *bev, short events, void *ctx) {
    client_data_t *client = (client_data_t *)ctx;

    if (events & BEV_EVENT_EOF) {
        printf("Client disconnected\n");
    } else if (events & BEV_EVENT_ERROR) {
        printf("Got an error on the connection: %s\n", strerror(errno));
    }

    if (client) {
        free(client);
    }

    bufferevent_free(bev);
}

// 【回调3】当有新客户端连接时，libevent 自动调用此函数
void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd,
                    struct sockaddr *address, int socklen, void *ctx) {
    printf("New client connected\n");
    
    struct event_base *base = evconnlistener_get_base(listener);
    
    // 为新的客户端连接创建 bufferevent
    struct bufferevent *bev = bufferevent_socket_new(
        base, fd, BEV_OPT_CLOSE_ON_FREE);
    
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!\n");
        evutil_closesocket(fd);
        return;
    }
    
    // 分配并初始化客户端数据
    client_data_t *client = (client_data_t *)malloc(sizeof(client_data_t));
    if (!client) {
        fprintf(stderr, "Error allocating client data!\n");
        bufferevent_free(bev);
        return;
    }
    client->bev = bev;
    client->recv_len = 0;
    memset(client->recv_buffer, 0, sizeof(client->recv_buffer));

    bufferevent_setcb(bev, read_cb, NULL, event_cb, client); //设置回调函数
    
    // 启用读写事件监听
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

// 【回调4】当监听器发生错误时，libevent 自动调用此函数
void accept_error_cb(struct evconnlistener *listener, void *ctx) {
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Got an error %d (%s) on the listener. "
            "Shutting down.\n", err, evutil_socket_error_to_string(err));
    
    event_base_loopexit(base, NULL);
}

int main() {
    struct event_base *base;
    struct evconnlistener *listener;
    struct sockaddr_in sin;

    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }
    
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(MODBUS_SERVER_PORT);
    sin.sin_addr.s_addr = htonl(0); // INADDR_ANY

    listener = evconnlistener_new_bind(base, accept_conn_cb,NULL,LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,-1,
        (struct sockaddr*)&sin, sizeof(sin));
    if (!listener) {
        fprintf(stderr, "Could not create a listener!\n");
        return 1;
    }
   
    printf("Modbus TCP Server listening on port %d...\n", MODBUS_SERVER_PORT);
    
    event_base_dispatch(base);
    
    evconnlistener_free(listener);
    event_base_free(base);
    
    printf("Server shutdown\n");
    return 0;
}