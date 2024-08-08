#include "ws_client.h"
#include <websocketpp/common/asio_ssl.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include "assertion.h"

namespace app {

WSClient::WSClient(const std::string_view& addr, const std::string_view& subs_msg, const std::string_view& cancel_subs_msg)
    : addr_(addr), subs_msg_(subs_msg), cancel_subs_msg_(cancel_subs_msg) {
    client_.init_asio();
    client_.set_tls_init_handler([](websocketpp::connection_hdl) {
        return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
    });

    client_.set_open_handler([this](const websocketpp::connection_hdl hdl) { onSubscription(hdl); });
    client_.set_message_handler([this](const websocketpp::connection_hdl hdl, app_tls_client::message_ptr msg) { onMessage(hdl, msg); });
    client_.set_fail_handler([this](const websocketpp::connection_hdl hdl) { onFail(hdl); });

    dpdk_init();  // Initialize DPDK
}

void WSClient::dpdk_init() {
    // DPDK initialization code, adjust according to specific requirements
    const uint16_t nb_ports = rte_eth_dev_count_avail();
    if (nb_ports == 0) {
        rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");
    }

    port_id = 0;  // Assume using the first NIC port
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", 8192, 250, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == NULL) {
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    }

    struct rte_eth_conf port_conf = {0};
    int                 ret = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (ret != 0) {
        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n", ret, port_id);
    }

    rte_eth_dev_start(port_id);
}

void WSClient::dpdk_send(const std::string& msg) {
    struct rte_mbuf* mbuf = rte_pktmbuf_alloc(mbuf_pool);
    if (!mbuf) {
        printf("Failed to allocate mbuf\n");
        return;
    }

    char* data = rte_pktmbuf_append(mbuf, msg.size());
    if (!data) {
        rte_pktmbuf_free(mbuf);
        printf("Failed to append data to mbuf\n");
        return;
    }

    rte_memcpy(data, msg.data(), msg.size());

    int sent = rte_eth_tx_burst(port_id, 0, &mbuf, 1);
    if (sent != 1) {
        rte_pktmbuf_free(mbuf);
        printf("Failed to send packet\n");
    }
}

void WSClient::start() {
    websocketpp::lib::error_code   ec;
    app_tls_client::connection_ptr con = client_.get_connection(addr_, ec);
    if (ec) {
        printf("connecting failed :[%s]\n", ec.message().c_str());
        return;
    }
    client_.connect(con);
    client_.run();
}

void WSClient::stop() {
    client_.stop();
}

void WSClient::onSubscription(websocketpp::connection_hdl hdl) {
    printf("sending subscription msg\n");
    ASSERT_LOG(!subs_msg_.empty(), "subscription msg is empty");
    dpdk_send(subs_msg_);  // Send subscription message via DPDK
}

void WSClient::onCancelSubscripton(websocketpp::connection_hdl hdl) {
    printf("sending cancel subscription msg\n");
    ASSERT_LOG(!cancel_subs_msg_.empty(), "cancel subscription msg is empty");
    dpdk_send(cancel_subs_msg_);  // Send cancel subscription message via DPDK
}

void WSClient::onFail(websocketpp::connection_hdl hdl) {
    printf("Connection failed\n");
}

}  // namespace app
