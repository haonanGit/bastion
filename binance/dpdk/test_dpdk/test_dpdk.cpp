#include <arpa/inet.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ip.h>
#include <rte_kni.h>
#include <rte_mbuf.h>
#include <rte_tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PKT_BURST 32          // 每次从网卡接收的最大数据包数量
#define MEMPOOL_CACHE_SIZE 256    // 内存池缓存大小
#define NB_MBUF 8192              // 内存池中mbuf的数量
#define SERVER_IP "192.168.1.10"  // KNI接口的IP地址
#define SERVER_PORT 12345         // 监听端口

// 默认端口配置
static const struct rte_eth_conf port_conf_default = {
    .rxmode =
        {
            .split_hdr_size = 0,
            .offloads = RTE_ETH_RX_OFFLOAD_CHECKSUM,
        },
};

struct rte_kni* kni;  // KNI接口指针

// 初始化并配置指定的以太网端口
static void init_port(uint16_t port_id, struct rte_mempool* mbuf_pool) {
    struct rte_eth_conf     port_conf = port_conf_default;
    const uint16_t          rx_rings = 1, tx_rings = 1;  // 设置接收和发送队列数目
    uint16_t                nb_rxd = 128, nb_txd = 512;  // 设置接收和发送队列的大小
    int                     retval;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_txconf   txconf;

    if (!rte_eth_dev_is_valid_port(port_id))
        rte_exit(EXIT_FAILURE, "Invalid port id %u\n", port_id);

    retval = rte_eth_dev_info_get(port_id, &dev_info);
    if (retval != 0)
        rte_exit(EXIT_FAILURE, "Error getting device info: %s\n", strerror(-retval));

    retval = rte_eth_dev_configure(port_id, rx_rings, tx_rings, &port_conf);
    if (retval != 0)
        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n", retval, port_id);

    retval = rte_eth_rx_queue_setup(port_id, 0, nb_rxd, rte_eth_dev_socket_id(port_id), NULL, mbuf_pool);
    if (retval < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n", retval, port_id);

    retval = rte_eth_tx_queue_setup(port_id, 0, nb_txd, rte_eth_dev_socket_id(port_id), &txconf);
    if (retval < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n", retval, port_id);

    retval = rte_eth_dev_start(port_id);
    if (retval < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n", retval, port_id);

    // 启用混杂模式
    rte_eth_promiscuous_enable(port_id);
}

// 初始化KNI接口
static void init_kni(uint16_t port_id) {
    struct rte_kni_conf conf;
    memset(&conf, 0, sizeof(conf));
    snprintf(conf.name, RTE_KNI_NAMESIZE, "vEth%d", port_id);  // 设置KNI接口名称
    conf.core_id = rte_lcore_id();                             // 绑定到当前lcore
    conf.group_id = port_id;                                   // 设置组ID
    conf.mbuf_size = RTE_MBUF_DEFAULT_BUF_SIZE;

    // 创建KNI接口
    kni =
        rte_kni_alloc(rte_pktmbuf_pool_create("MBUF_POOL", NB_MBUF, MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id()), &conf, NULL);
    if (!kni)
        rte_exit(EXIT_FAILURE, "Could not create KNI for port %d\n", port_id);
}

// 初始化KNI接口的IP地址
static void configure_kni_ip() {
    system("ip link set dev vEth0 up");
    system("ip addr add " SERVER_IP "/24 dev vEth0");
    system("ip route add default via 192.168.1.1 dev vEth0");
}

// 处理接收到的数据包
void process_packet(struct rte_mbuf* mbuf) {
    struct rte_ether_hdr* eth_hdr;       // 以太网帧头
    struct rte_ipv4_hdr*  ipv4_hdr;      // IPv4头
    struct rte_tcp_hdr*   tcp_hdr;       // TCP头
    char*                 payload;       // 数据载荷
    uint16_t              eth_type;      // 以太网类型
    uint16_t              ipv4_hdr_len;  // IPv4头长度
    uint16_t              tcp_hdr_len;   // TCP头长度

    // 提取以太网帧头
    eth_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr*);
    eth_type = rte_be_to_cpu_16(eth_hdr->ether_type);

    // 只处理IPv4数据包
    if (eth_type != RTE_ETHER_TYPE_IPV4)
        return;

    // 提取IPv4头
    ipv4_hdr = (struct rte_ipv4_hdr*)(eth_hdr + 1);
    ipv4_hdr_len = (ipv4_hdr->version_ihl & RTE_IPV4_HDR_IHL_MASK) * RTE_IPV4_IHL_MULTIPLIER;

    // 只处理TCP数据包
    if (ipv4_hdr->next_proto_id != IPPROTO_TCP)
        return;

    // 提取TCP头
    tcp_hdr = (struct rte_tcp_hdr*)((unsigned char*)ipv4_hdr + ipv4_hdr_len);
    tcp_hdr_len = (tcp_hdr->data_off >> 4) * 4;  // 直接使用固定的乘数4，因为数据偏移字段是一个4位字段，它表示TCP头的32位字的数量

    // 提取数据载荷
    payload = (char*)((unsigned char*)tcp_hdr + tcp_hdr_len);
    uint16_t payload_len = rte_be_to_cpu_16(ipv4_hdr->total_length) - ipv4_hdr_len - tcp_hdr_len;

    // 打印接收到的消息
    printf("Received message: %.*s\n", payload_len, payload);
}

int main(int argc, char* argv[]) {
    uint16_t            port_id = 0;  // DPDK端口ID
    struct rte_mempool* mbuf_pool;    // 内存池

    // 初始化DPDK环境
    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

    // 创建内存池
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NB_MBUF, MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    // 初始化以太网端口
    init_port(port_id, mbuf_pool);

    // 初始化KNI接口
    init_kni(port_id);

    // 在系统中配置KNI接口的IP地址
    configure_kni_ip();

    struct rte_mbuf* bufs[MAX_PKT_BURST];  // 用于接收数据包的数组
    uint16_t         nb_rx;

    // 主循环：从ens6网卡接收数据包并处理
    while (1) {
        // 从网卡接收数据包
        nb_rx = rte_eth_rx_burst(port_id, 0, bufs, MAX_PKT_BURST);
        for (int i = 0; i < nb_rx; i++) {
            // 处理接收到的数据包
            process_packet(bufs[i]);
            // 释放mbuf
            rte_pktmbuf_free(bufs[i]);
        }
    }

    return 0;
}
