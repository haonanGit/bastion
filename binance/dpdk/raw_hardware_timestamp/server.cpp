#include <errno.h>
#include <fcntl.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

// 启用套接字的时间戳选项
void enable_timestamping(int sockfd) {
    int timestamp_flags = SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE | SOF_TIMESTAMPING_SOFTWARE;
    if (setsockopt(sockfd, SOL_SOCKET, SO_TIMESTAMPING, &timestamp_flags, sizeof(timestamp_flags)) < 0) {
        perror("setsockopt SO_TIMESTAMPING");
        exit(EXIT_FAILURE);
    }
}

// 将套接字设置为阻塞模式
void set_blocking_mode(int sockfd, bool blocking) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }

    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }

    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL");
        exit(EXIT_FAILURE);
    }
}

void receive_and_process_messages(int sockfd) {
    struct msghdr    msg;
    struct iovec     iov;
    char             buffer[2048];
    char             control[1024];
    struct cmsghdr*  cmsg;
    struct timespec* ts;
    ssize_t          len;

    memset(&msg, 0, sizeof(msg));
    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    while (true) {
        // 接收数据包
        len = recvmsg(sockfd, &msg, 0);
        if (len < 0) {
            perror("recvmsg");
            return;
        }

        std::cout << "Received message: " << buffer << std::endl;

        // 尝试从错误队列接收时间戳消息
        for (int retries = 0; retries < 5; retries++) {
            len = recvmsg(sockfd, &msg, MSG_ERRQUEUE);
            if (len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::cout << "No timestamp available, retrying..." << std::endl;
                    usleep(200000);  // 等待 200 毫秒后重试
                    continue;
                } else {
                    perror("recvmsg");
                    return;
                }
            }

            // 处理控制消息以获取时间戳
            for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
                if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMPING) {
                    ts = (struct timespec*)CMSG_DATA(cmsg);
                    std::cout << "SW timestamp: " << ts[0].tv_sec << "." << ts[0].tv_nsec << std::endl;
                    std::cout << "RHW timestamp: " << ts[1].tv_sec << "." << ts[1].tv_nsec << std::endl;
                    std::cout << "HW timestamp: " << ts[2].tv_sec << "." << ts[2].tv_nsec << std::endl;
                    break;  // 成功获取到时间戳后跳出重试循环
                }
            }
            break;
        }
    }
}

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return 1;
    }

    // 绑定到指定端口
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(12345);

    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return 1;
    }

    std::cout << "Server is running, waiting for messages..." << std::endl;

    // 启用时间戳
    enable_timestamping(sockfd);

    // 将套接字设置为阻塞模式
    set_blocking_mode(sockfd, true);

    // 接收并处理消息
    receive_and_process_messages(sockfd);

    close(sockfd);
    return 0;
}
