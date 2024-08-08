#ifndef GETOPENORDERSBYINSTRUMENT
#define GETOPENORDERSBYINSTRUMENT

#include "ws_client.h"

namespace app {

class GetOpenOrdersByInstrument : public WSClient {
public:
  GetOpenOrdersByInstrument() = default;
  ~GetOpenOrdersByInstrument() = default;
  GetOpenOrdersByInstrument(const std::string_view &addr,
                            const std::string_view &subs_msg,
                            const std::string_view &cancel_subs_msg)
      : WSClient(addr, subs_msg, cancel_subs_msg) {}
  void onAuth(websocketpp::connection_hdl hdl) override;
  void onMessage(websocketpp::connection_hdl hdl,
                 app_tls_client::message_ptr msg) override;
};
} // namespace app

#endif // GETOPENORDERSBYINSTRUMENT