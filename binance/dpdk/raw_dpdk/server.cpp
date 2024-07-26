#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <chrono>
#include <iostream>

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 1

long long getCurrentTimeNs() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

static long long total_time = 0;
static int       count = 0;

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

void receiveAndEchoMessages(uint16_t tx_port_id, uint16_t rx_port_id, struct rte_mempool* mbuf_pool) {
    struct rte_mbuf* bufs[BURST_SIZE];

    // struct rte_eth_dev_tx_buffer* buffer;
    // buffer = (struct rte_eth_dev_tx_buffer*)rte_malloc("tx_buffer", RTE_ETH_TX_BUFFER_SIZE(BURST_SIZE), 0);
    // if (buffer == NULL)
    //     rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n", (unsigned)tx_port_id);

    // rte_eth_tx_buffer_init(buffer, BURST_SIZE);
    bool end = false;

    while (!end) {
        int ret = 0;
        std::cout << "run server!!!!!!!!!" << std::endl;
        while ((ret = rte_eth_rx_burst(rx_port_id, 0, bufs, BURST_SIZE)) == 0) {
            // Busy wait until a packet is received
        }

        for (int j = 0; j < ret; ++j) {
            struct rte_mbuf* mbuf = bufs[j];
            char*            data = rte_pktmbuf_mtod(mbuf, char*);

            // Echo the original message back without modification
            if (rte_pktmbuf_pkt_len(mbuf) >= 4 && strncmp(data, "dpdk", 4) == 0) {
                // int sent = rte_eth_tx_buffer(tx_port_id, 0, buffer, mbuf);
                // if (sent < BURST_SIZE) {
                //     rte_eth_tx_buffer_flush(tx_port_id, 0, buffer);
                // }
                int64_t send_time = std::stoll(data + 4);  // Assuming the timestamp starts after "dpdk"
                auto    current_time = getCurrentTimeNs();
                total_time += current_time - send_time;
                ++count;
            }
            if (rte_pktmbuf_pkt_len(mbuf) >= 3 && strncmp(data, "end", 3) == 0) {
                end = true;
                break;
            }
            rte_pktmbuf_free(mbuf);  // Free the mbuf after processing
        }
    }

    // rte_eth_tx_buffer_flush(tx_port_id, 0, buffer);
    std::cout << "average time ns:" << (static_cast<double>(total_time) / count) << std::endl;
}

void runServer() {
    int         argc = 2;
    const char* argv[] = {"server", "-l 0-1"};
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
    // ret = rte_eth_dev_configure(rx_port_id, 1, 0, &port_conf_default);
    // if (ret < 0) {
    //     rte_exit(EXIT_FAILURE, "Error with transmit port configuration\n");
    // }
    // ret = rte_eth_tx_queue_setup(tx_port_id, 0, 128, rte_eth_dev_socket_id(tx_port_id), NULL);
    // if (ret < 0) {
    //     rte_exit(EXIT_FAILURE, "Error with transmit queue setup\n");
    // }

    // ret = rte_eth_dev_start(tx_port_id);
    // if (ret < 0) {
    //     rte_exit(EXIT_FAILURE, "Error with starting transmit port\n");
    // }

    // Initialize the receive port
    ret = rte_eth_dev_configure(rx_port_id, 1, 0, &port_conf_default);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Error with receive port configuration\n");
    }

    ret = rte_eth_rx_queue_setup(rx_port_id, 0, 128, rte_eth_dev_socket_id(rx_port_id), NULL, mbuf_pool);

    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Error with receive queue setup\n");
    }

    ret = rte_eth_dev_start(rx_port_id);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Error with starting receive port\n");
    }

    receiveAndEchoMessages(tx_port_id, rx_port_id, mbuf_pool);

    // rte_eth_dev_stop(tx_port_id);
    // rte_eth_dev_close(tx_port_id);
    rte_eth_dev_stop(rx_port_id);
    rte_eth_dev_close(rx_port_id);
    rte_eal_cleanup();
}

int main() {
    runServer();
    return 0;
}
