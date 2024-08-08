#include <arpa/inet.h>  // 包含inet_pton和其他网络函数
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 12345
#define BUFFER_SIZE 1024

int main() {
    int                sockfd, newsockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t          cliaddr_len;
    char               buffer[BUFFER_SIZE];

    // 创建套接字
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    // 使用inet_pton代替inet_addr来转换IPv4地址
    if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(sockfd);
        return 1;
    }

    // 绑定套接字到地址和端口
    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return 1;
    }

    // 监听
    if (listen(sockfd, 5) < 0) {
        perror("listen failed");
        close(sockfd);
        return 1;
    }

    printf("Server is listening on 127.0.0.1:%d\n", PORT);

    // 处理连接请求
    while (1) {
        cliaddr_len = sizeof(cliaddr);

        // 接受连接
        if ((newsockfd = accept(sockfd, (struct sockaddr*)&cliaddr, &cliaddr_len)) < 0) {
            perror("accept failed");
            close(sockfd);
            return 1;
        }

        printf("Accepted a connection\n");

        // 接收消息
        ssize_t n = read(newsockfd, buffer, BUFFER_SIZE - 1);
        if (n < 0) {
            perror("read failed");
        } else {
            buffer[n] = '\0';  // 确保字符串以null结尾
            printf("Received message: %s\n", buffer);
        }

        // 关闭连接
        close(newsockfd);
    }

    close(sockfd);
    return 0;
}
