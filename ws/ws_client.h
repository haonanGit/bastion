#ifndef WSCLIENT_H
#define WSCLIENT_H

#include <assert.h>
#include <string>
#include <string_view>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

using app_tls_client =
    websocketpp::client<websocketpp::config::asio_tls_client>;
// using app_tls_client = websocketpp::client<websocketpp::config::asio_client>;

namespace app {

class WSClient {
private:
  app_tls_client client_;
  std::string addr_; // server address
  std::string subs_msg_;
  std::string cancel_subs_msg_;

public:
  WSClient() = default;
  ~WSClient() = default;
  WSClient(const std::string_view &addr, const std::string_view &subs_msg,
           const std::string_view &cancel_subs_msg);
  virtual void start();
  virtual void stop();
  virtual void onAuth(websocketpp::connection_hdl hdl){};
  virtual void onSubscription(websocketpp::connection_hdl hdl);
  virtual void onCancelSubscripton(websocketpp::connection_hdl hdl);
  virtual void onMessage(websocketpp::connection_hdl hdl,
                         app_tls_client::message_ptr msg) = 0;
  virtual void onFail(websocketpp::connection_hdl hdl);
  inline void send(websocketpp::connection_hdl hdl, const string_view &msg) {
    client_.send(hdl, msg, websocketpp::frame::opcode::text);
  }
};

} // namespace app

#endif // WSCLIENT_H