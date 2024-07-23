
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
        // auto ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);

        // // 启用详细的 OpenSSL 调试信息
        // SSL_CTX_set_info_callback(ctx->native_handle(), [](const SSL* ssl, int type, int val) {
        //     const char* str = SSL_state_string_long(ssl);
        //     printf("SSL Info: type = %d, val = %d, state = %s\n", type, val, str);

        //     if (type == 4098 && val == -1) {
        //         // 打印 OpenSSL 错误堆栈
        //         unsigned long err;
        //         while ((err = ERR_get_error()) != 0) {
        //             char* err_str = ERR_error_string(err, nullptr);
        //             printf("OpenSSL Error: %s\n", err_str);
        //         }
        //     }
        // });

        // ctx->set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::no_sslv3
        // |
        //                  boost::asio::ssl::context::single_dh_use);
        // ctx->set_verify_mode(boost::asio::ssl::verify_peer);
        // ctx->set_default_verify_paths();

        // // 设置验证回调
        // ctx->set_verify_callback(boost::asio::ssl::rfc2818_verification("fstream.binance.com"));

        // return ctx;
    });

    client_.set_open_handler([this](const websocketpp::connection_hdl hdl) { onSubscription(hdl); });
    client_.set_message_handler([this](const websocketpp::connection_hdl hdl, app_tls_client::message_ptr msg) { onMessage(hdl, msg); });
    client_.set_fail_handler([this](const websocketpp::connection_hdl hdl) { onFail(hdl); });
}

// WSClient::WSClient(const std::string_view& addr, const std::string_view& subs_msg, const std::string_view& cancel_subs_msg)
//     : addr_(addr), subs_msg_(subs_msg), cancel_subs_msg_(cancel_subs_msg) {
//     client_.init_asio();
//     client_.set_open_handler([this](const websocketpp::connection_hdl hdl) { onSubscription(hdl); });
//     client_.set_message_handler([this](const websocketpp::connection_hdl hdl, app_tls_client::message_ptr msg) { onMessage(hdl, msg); });
//     client_.set_fail_handler([this](const websocketpp::connection_hdl hdl) { onFail(hdl); });
// }

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
    client_.send(hdl, subs_msg_, websocketpp::frame::opcode::text);
}

void WSClient::onCancelSubscripton(websocketpp::connection_hdl hdl) {
    printf("sending cancel subscription msg\n");
    ASSERT_LOG(!cancel_subs_msg_.empty(), "cancel subscription msg is empty");
    client_.send(hdl, cancel_subs_msg_, websocketpp::frame::opcode::text);
}

void WSClient::onFail(websocketpp::connection_hdl hdl) {
    printf("Connection failed\n");
}

}  // namespace app