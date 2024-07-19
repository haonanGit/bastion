#include <arpa/inet.h>  // 包含 inet_addr 函数
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return 1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(12345);

    int count = 0;

    while (count < 10) {
        ++count;
        std::string message = "current count is : " + std::to_string(count);
        if (sendto(sockfd, message.c_str(), strlen(message.c_str()), 0, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            perror("sendto failed");
            close(sockfd);
            return 1;
        }

        std::cout << "Message sent to server:[" << message << "]" << std::endl;
        sleep(1);  // 每秒发送一次消息
    }

    close(sockfd);
    return 0;
}
