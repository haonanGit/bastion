#ifndef AGGREGATE_TRADE_STREAMS_H
#define AGGREGATE_TRADE_STREAMS_H

#include "ws_client.h"

namespace app {

class AggregateTradeStreamsClient : public WSClient {
public:
    AggregateTradeStreamsClient() = default;
    ~AggregateTradeStreamsClient() = default;
    AggregateTradeStreamsClient(const std::string_view& addr, const std::string_view& subs_msg, const std::string_view& cancel_subs_msg)
        : WSClient(addr, subs_msg, cancel_subs_msg) {}
    void onMessage(websocketpp::connection_hdl hdl, app_client::message_ptr msg) override;
};
}  // namespace app

#endif  // AGGREGATE_TRADE_STREAMS_H