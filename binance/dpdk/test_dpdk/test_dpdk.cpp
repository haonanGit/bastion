#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <iostream>

#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static const struct rte_eth_conf port_conf_default = {
    .rxmode =
        {
            .mtu = RTE_ETHER_MTU,
        },
};

static inline int port_init(uint16_t port, struct rte_mempool* mbuf_pool) {
    struct rte_eth_conf port_conf = port_conf_default;
    const uint16_t      rx_rings = 1, tx_rings = 1;
    int                 retval;
    uint16_t            q;

    if (port >= rte_eth_dev_count_avail())
        return -1;

    // Configure the Ethernet device
    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval != 0)
        return retval;

    // Allocate and set up RX queues per Ethernet port
    for (q = 0; q < rx_rings; q++) {
        retval = rte_eth_rx_queue_setup(port, q, 1024, rte_eth_dev_socket_id(port), NULL, mbuf_pool);
        if (retval < 0)
            return retval;
    }

    // Allocate and set up TX queues per Ethernet port
    for (q = 0; q < tx_rings; q++) {
        retval = rte_eth_tx_queue_setup(port, q, 1024, rte_eth_dev_socket_id(port), NULL);
        if (retval < 0)
            return retval;
    }

    // Start the Ethernet port
    retval = rte_eth_dev_start(port);
    if (retval < 0)
        return retval;

    // Display the port MAC address
    struct rte_ether_addr addr;
    rte_eth_macaddr_get(port, &addr);
    printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n", port, addr.addr_bytes[0],
           addr.addr_bytes[1], addr.addr_bytes[2], addr.addr_bytes[3], addr.addr_bytes[4], addr.addr_bytes[5]);

    // Enable RX in promiscuous mode for the Ethernet device
    rte_eth_promiscuous_enable(port);

    return 0;
}

int main(int argc, char* argv[]) {
    struct rte_mempool* mbuf_pool;
    unsigned            nb_ports;
    uint16_t            portid;

    // Initialize the Environment Abstraction Layer (EAL)
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    argc -= ret;
    argv += ret;

    // Check that there is an even number of ports to send/receive on
    nb_ports = rte_eth_dev_count_avail();
    if (nb_ports < 1)
        rte_exit(EXIT_FAILURE, "Error: number of ports must be even\n");

    // Creates a new mempool in memory to hold the mbufs
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    // Initialize all ports
    RTE_ETH_FOREACH_DEV(portid) {
        if (port_init(portid, mbuf_pool) != 0)
            rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n", portid);
    }

    // Call lcore_main on the master core only
    rte_eal_mp_remote_launch(
        [](void*) -> int {
            while (1)
                ;
            return 0;
        },
        NULL, CALL_MAIN);
    unsigned lcore_id;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0)
            return -1;
    }

    return 0;
}
