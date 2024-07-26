#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 10

std::atomic<long long> total_rtt{0};

static const struct rte_eth_conf port_conf_default = {
    .rxmode =
        {
            .mq_mode = ETH_MQ_RX_NONE,  // 接收队列模式
        },
    .txmode =
        {
            .mq_mode = ETH_MQ_TX_NONE,
        },
};

long long getCurrentTimeNs() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

void sendMessages(uint16_t port_id, int message_count, struct rte_mempool* mbuf_pool) {
    std::cout << "client start!!!!!!!!!!!" << std::endl;
    struct rte_eth_dev_tx_buffer* buffer;
    buffer = (struct rte_eth_dev_tx_buffer*)rte_malloc("tx_buffer", RTE_ETH_TX_BUFFER_SIZE(BURST_SIZE), 0);
    if (buffer == NULL)
        rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n", (unsigned)port_id);

    rte_eth_tx_buffer_init(buffer, BURST_SIZE);

    for (int i = 0; i < message_count; ++i) {
        struct rte_mbuf* mbuf = rte_pktmbuf_alloc(mbuf_pool);
        if (mbuf == nullptr) {
            rte_exit(EXIT_FAILURE, "Error with allocating mbuf\n");
        }

        char data[40];  // Ensure enough space for "dpdk" prefix and timestamp
        auto send_time = getCurrentTimeNs();
        snprintf(data, sizeof(data), "dpdk%lld", send_time);

        char* packet_data = rte_pktmbuf_mtod(mbuf, char*);
        strcpy(packet_data, data);
        mbuf->data_len = strlen(data);
        mbuf->pkt_len = mbuf->data_len;

        int sent = rte_eth_tx_buffer(port_id, 0, buffer, mbuf);
        if (sent < BURST_SIZE) {
            rte_eth_tx_buffer_flush(port_id, 0, buffer);
        }
        std::cout << "send :" << sent << std::endl;

        rte_pktmbuf_free(mbuf);  // Free the mbuf after sending
    }

    // Send end message
    struct rte_mbuf* end_mbuf = rte_pktmbuf_alloc(mbuf_pool);
    if (end_mbuf == nullptr) {
        rte_exit(EXIT_FAILURE, "Error with allocating end mbuf\n");
    }

    char* end_data = rte_pktmbuf_mtod(end_mbuf, char*);
    strcpy(end_data, "end");
    end_mbuf->data_len = strlen("end");
    end_mbuf->pkt_len = end_mbuf->data_len;

    int sent = rte_eth_tx_buffer(port_id, 0, buffer, end_mbuf);
    if (sent < BURST_SIZE) {
        rte_eth_tx_buffer_flush(port_id, 0, buffer);
    }

    rte_pktmbuf_free(end_mbuf);  // Free the mbuf after sending

    rte_eth_tx_buffer_flush(port_id, 0, buffer);
    rte_free(buffer);  // Free the allocated tx buffer
}

// void receiveMessages(uint16_t port_id, const int& message_count) {
//     struct rte_mbuf* bufs[BURST_SIZE];
//     int              count = 0;
//     bool             end = false;
//     while (!end) {
//         int ret = 0;
//         while ((ret = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE)) == 0) {
//             // Busy wait until a packet is received
//         }
//         auto receive_time = getCurrentTimeNs();

//         for (int j = 0; j < ret; ++j) {
//             struct rte_mbuf* mbuf = bufs[j];
//             char*            data = rte_pktmbuf_mtod(mbuf, char*);
//             const auto&      data_len = rte_pktmbuf_pkt_len(mbuf);

//             // Ensure the data length is sufficient to contain "dpdk"
//             if (data_len > 4 && strncmp(data, "dpdk", 4) == 0) {
//                 int64_t send_time = std::stoll(data + 4);  // Assuming the timestamp starts after "dpdk"
//                 total_rtt += receive_time - send_time;
//             } else if (data_len >= 3 && strncmp(data, "end", 3) == 0) {
//                 end = true;
//                 break;
//             }
//             rte_pktmbuf_free(mbuf);  // Free the mbuf after processing
//         }
//     }
// }

void runClient(int message_count) {
    int         argc = 5;
    const char* argv[] = {"client", "-l", "0-1", "--file-prefix", "client_prefix", "--socket-mem=1024,1024"};
    int         ret = rte_eal_init(argc, const_cast<char**>(argv));
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    }

    uint16_t tx_port_id = 0;  // Transmit port ID
    uint16_t rx_port_id = 0;  // Receive port ID

    struct rte_mempool* mbuf_pool =
        rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * 2, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

    // Initialize the transmit port
    ret = rte_eth_dev_configure(tx_port_id, 1, 1, &port_conf_default);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Error with transmit port configuration\n");
    }
    ret = rte_eth_tx_queue_setup(tx_port_id, 0, 128, rte_eth_dev_socket_id(tx_port_id), NULL);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Error with transmit queue setup\n");
    }

    ret = rte_eth_dev_start(tx_port_id);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Error with starting transmit port\n");
    }

    // Initialize the receive port
    // ret = rte_eth_dev_configure(rx_port_id, 1, 1, &port_conf_default);
    // if (ret < 0) {
    //     rte_exit(EXIT_FAILURE, "Error with receive port configuration\n");
    // }
    // ret = rte_eth_rx_queue_setup(rx_port_id, 0, 128, rte_eth_dev_socket_id(rx_port_id), NULL, mbuf_pool);
    // if (ret < 0) {
    //     rte_exit(EXIT_FAILURE, "Error with receive queue setup\n");
    // }

    // ret = rte_eth_dev_start(rx_port_id);
    // if (ret < 0) {
    //     rte_exit(EXIT_FAILURE, "Error with starting receive port\n");
    // }

    std::thread sender_thread(sendMessages, tx_port_id, message_count, mbuf_pool);
    // std::thread receiver_thread(receiveMessages, rx_port_id, message_count);

    sender_thread.join();
    // receiver_thread.join();

    // std::cout << "Total RTT: " << total_rtt << " ns" << std::endl;
    // std::cout << "Average RTT: " << total_rtt / message_count << " ns" << std::endl;

    rte_eth_dev_stop(tx_port_id);
    rte_eth_dev_close(tx_port_id);
    // rte_eth_dev_stop(rx_port_id);
    // rte_eth_dev_close(rx_port_id);
    // rte_eal_cleanup();
}

int main() {
    int message_count = 10000;
    runClient(message_count);
    return 0;
}
