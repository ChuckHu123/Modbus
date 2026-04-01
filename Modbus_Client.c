#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

// Modbus 参数配置
#define MODBUS_SERVER_IP "193.169.202.29"  // HSL 仿真软件所在 IP
#define MODBUS_SERVER_PORT 502        // Modbus TCP 默认端口
#define SLAVE_ID 1                    // 从站 ID

// Modbus 功能码
#define FC_READ_HOLDING_REGISTERS 0x03

#define BUFFER_SIZE 1024

int main() {
    //
    char ip_str[16];
    int port;
    printf("Enter Server IP: ");
    scanf("%s", ip_str);
    printf("Enter Server Port: ");
    scanf("%d", &port);
    while (getchar() != '\n');// 清空输入缓冲区

    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully\n");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_str, &server_addr.sin_addr) <= 0) {
        printf("Invalid IP address format.\n");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to the server.\n");

    int epoll_fd = epoll_create1(0);
    struct epoll_event event, events[2];
    event.events = EPOLLIN;
    event.data.fd = 0;// 监控来自键盘的标准输入
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event);

    event.events = EPOLLIN;
    event.data.fd = client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);//监控服务端的消息
//
    printf("Enter message (type 'quit' to exit): ");
    fflush(stdout);
    
    unsigned char request[] = {
        0x00, 0x01, // Transaction ID
        0x00, 0x00, // Protocol ID (0 = Modbus)
        0x00, 0x06, // Length (Unit ID + PDU = 1+1+2+2 = 6)
        0x01,       // Unit ID (从站地址)
        0x03,       // Function Code (Read Holding Registers)
        0x00, 0x00, // Starting Address High/Low
        0x00, 0x01  // Quantity of Registers High/Low (读1个)
    };

    if (send(client_fd, request, sizeof(request), 0) < 0) {
        perror("send failed");
        close(client_fd);
        close(epoll_fd);
        return 0;
    }

    int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);

    if (bytes_received > 0) {
        printf("Received %d bytes: ", bytes_received);
        for (int i = 0; i < bytes_received; i++) {
            printf("%02X ", buffer[i]);
        }
        printf("\n");

        // 简易解析：03 功能码响应的数据在第 9 字节开始（前 9 字节是 Header, FC, Byte Count）
        if (bytes_received >= 9 && buffer[7] == 0x03) {
            int byte_count = buffer[8];
            int value = (buffer[9] << 8) | buffer[10];
            printf("Register 0 Value: %d\n", value);
        }
    }
/*
    while (1) {
        int num_events = epoll_wait(epoll_fd, events, 2, -1);
        for(int i = 0; i < num_events; i++){
            if (events[i].data.fd == 0) {// 处理标准输入
                if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                    if (strcmp(buffer, "quit\n") == 0) {
                        printf("Exiting the client.\n");
                        close(client_fd);
                        close(epoll_fd);
                        return 0;
                    }else{
                        buffer[strcspn(buffer, "\n")] = 0;
                        if (strlen(buffer)==0){continue;}
                        ssize_t send_len = send(client_fd, buffer, strlen(buffer), 0);
                        if (send_len < 0) {
                            perror("send failed");
                            close(client_fd);
                            close(epoll_fd);
                            return 0;
                        }
                    }
                }else{
                    perror("fgets failed");
                    close(client_fd);
                    close(epoll_fd);
                    return 0;
                }
            }else{// 处理服务器消息
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t recv_len = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
                if (recv_len <= 0) {
                    if (recv_len < 0) {
                        perror("Recv failed");
                    } else {
                        printf("Server disconnected.\n");
                    }
                    close(client_fd);
                    close(epoll_fd);
                    return 0;
                }
                printf("Received from server: %s\n", buffer);
                printf("Enter message (type 'quit' to exit): ");
                fflush(stdout);
            }
        }

    }
    */
    close(client_fd);
    close(epoll_fd);
    printf("Client closed.\n");
    return 0;
}
