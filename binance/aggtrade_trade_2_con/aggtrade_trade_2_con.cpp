
#include <fstream>
#include "aggregate_trade_streams.h"
#include "common.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "vector"

int                  count = 0;
int                  threhold = INT_MAX;
static utils::Logger agg_logger;
static utils::Logger trade_logger;

namespace app {

void convertTimestampToDate(nlohmann::json& jsonObject) {
    // Aggregate Trade Streams
    if (jsonObject.contains("data") && !jsonObject["data"].is_null() && jsonObject["data"].contains("E") && !jsonObject["data"]["E"].is_null() &&
        jsonObject["data"].contains("T") && !jsonObject["data"]["T"].is_null()) {
        long long   E = jsonObject["data"]["E"];
        long long   T = jsonObject["data"]["T"];
        std::string E_date = common::timestampToDate(E, common::TimeUnit::Milliseconds);
        std::string T_date = common::timestampToDate(T, common::TimeUnit::Milliseconds);
        jsonObject["data"]["E"] = E_date;
        jsonObject["data"]["T"] = T_date;
    }
}

void AggregateTradeStreamsClient::onMessage(websocketpp::connection_hdl hdl, app_tls_client::message_ptr msg) {
    const auto&    recv_tm = common::getTimeStampNs();
    const auto&    ret_msg = msg->get_payload();
    nlohmann::json jsonObject = nlohmann::json::parse(ret_msg);
    // convertTimestampToDate(jsonObject);

    // jsonObject["received_t"] = common::timestampToDate(recv_tm, common::TimeUnit::Nanoseconds);
    // const auto& ret_msg = jsonObject.dump();
    // ++count;

    auto& logger = jsonObject["data"]["e"] == "trade" ? trade_logger : agg_logger;
    auto  mid_tm = common::getTimeStampNs();
    logger.info(ret_msg);
    auto        end_tm = common::getTimeStampNs();
    std::string info = "json:[" + std::to_string(mid_tm - recv_tm) + "]ns " + "log:[" + std::to_string(end_tm - mid_tm) + "] " + "total:[" +
                       std::to_string(end_tm - recv_tm) + "]";
    LOG_INFO(info);
}

}  // namespace app

void wbSocketThread(const std::string_view& addr, const std::string_view& sbus_msg, const std::string_view& cancel_subs_msg) {
    std::cout << "addr : " << addr << std::endl;
    app::AggregateTradeStreamsClient client(addr, sbus_msg, cancel_subs_msg);
    client.start();
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string s_threhold = argv[1];
        threhold = std::stoi(s_threhold);
    }

    // init log
    utils::Logger::Options opt_agg;
    opt_agg.name = "agg";
    opt_agg.file = "agg.txt";
    opt_agg.dir = "../log";
    opt_agg.is_async = true;
    opt_agg.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_agg.max_files = 50;                 // up to X GB total

    utils::Logger::Options opt_trade;
    opt_trade.name = "trade";
    opt_trade.file = "trade.txt";
    opt_trade.dir = "../log";
    opt_trade.is_async = true;
    opt_trade.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_trade.max_files = 50;                 // up to X GB total

    utils::Logger::Options opt_log;
    opt_log.name = "log";
    opt_log.file = "log.txt";
    opt_log.dir = "../log";
    opt_log.is_async = true;
    opt_log.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_log.max_files = 50;                 // up to X GB total

    utils::GlobalLog::ins().init(opt_log);
    agg_logger.init(opt_agg);
    trade_logger.init(opt_trade);

    // aggtrade
    std::string addr_agg = "wss://fstream.binance.com/stream?streams=xrpusdt@aggTrade/ethusdt@aggTrade";
    std::string subs_msg_agg = R"({"method":"SUBSCRIBE","params":["xrpusdt@aggTrade","ethusdt@aggTrade"],"id":1})";
    std::string cancel_subs_msg_agg = R"({"method":"UNSUBSCRIBE","params":["xrpusdt@aggTrade","ethusdt@aggTrade"],"id":1})";

    // trade
    std::string addr_agg_trade = "wss://fstream.binance.com/stream?streams=xrpusdt@trade/ethusdt@trade";
    std::string subs_msg_agg_trade = R"({"method":"SUBSCRIBE","params":["xrpusdt@trade","ethusdt@trade"],"id":2})";
    std::string cancel_subs_msg_agg_trade = R"({"method":"UNSUBSCRIBE","params":["xrpusdt@trade","ethusdt@trade"],"id":2})";

    std::vector<std::thread> ws_threads;

    ws_threads.emplace_back(wbSocketThread, addr_agg, subs_msg_agg, cancel_subs_msg_agg);
    ws_threads.emplace_back(wbSocketThread, addr_agg_trade, subs_msg_agg_trade, cancel_subs_msg_agg_trade);

    for (auto& t : ws_threads) {
        t.join();
    }

    return 0;
}