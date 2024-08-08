
#include <fstream>
#include "aggregate_trade_streams.h"
#include "common.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "vector"

int                  count = 0;
int                  threhold = INT_MAX;
static utils::Logger agg_logger_xrp;
static utils::Logger agg_logger_eth;
static utils::Logger trade_logger_xrp;
static utils::Logger trade_logger_eth;

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
    const auto& recv_tm = common::getTimeStampNs();
    // const auto&    ret_msg = msg->get_payload();
    nlohmann::json jsonObject = nlohmann::json::parse(msg->get_payload());
    convertTimestampToDate(jsonObject);

    jsonObject["received_t"] = common::timestampToDate(recv_tm, common::TimeUnit::Nanoseconds);
    const auto& ret_msg = jsonObject.dump();
    // ++count;

    utils::Logger logger;
    if (jsonObject["data"]["e"] == "trade") {
        logger = jsonObject["data"]["s"] == "XRPUSDT" ? trade_logger_xrp : trade_logger_eth;
    } else {
        logger = jsonObject["data"]["s"] == "XRPUSDT" ? agg_logger_xrp : agg_logger_eth;
    }
    // auto& logger = jsonObject["data"]["e"] == "trade" ? trade_logger : agg_logger;
    // auto  mid_tm = common::getTimeStampNs();
    logger.info(ret_msg);
    // auto        end_tm = common::getTimeStampNs();
    // std::string info = "json:[" + std::to_string(mid_tm - recv_tm) + "]ns " + "log:[" + std::to_string(end_tm - mid_tm) + "] " + "total:[" +
    //                    std::to_string(end_tm - recv_tm) + "]";
    // LOG_INFO(info);
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
    utils::Logger::Options opt_agg_xrp;
    opt_agg_xrp.name = "f_agg_xrp";
    opt_agg_xrp.file = "f_agg_xrp.txt";
    opt_agg_xrp.dir = "../log";
    opt_agg_xrp.is_async = true;
    opt_agg_xrp.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_agg_xrp.max_files = 50;                 // up to X GB total

    utils::Logger::Options opt_agg_eth;
    opt_agg_eth.name = "f_agg_eth";
    opt_agg_eth.file = "f_agg_eth.txt";
    opt_agg_eth.dir = "../log";
    opt_agg_eth.is_async = true;
    opt_agg_eth.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_agg_eth.max_files = 50;                 // up to X GB total

    utils::Logger::Options opt_trade_xrp;
    opt_trade_xrp.name = "f_trade_xrp";
    opt_trade_xrp.file = "f_trade_xrp.txt";
    opt_trade_xrp.dir = "../log";
    opt_trade_xrp.is_async = true;
    opt_trade_xrp.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_trade_xrp.max_files = 50;                 // up to X GB total

    utils::Logger::Options opt_trade_eth;
    opt_trade_eth.name = "f_trade_eth";
    opt_trade_eth.file = "f_trade_eth.txt";
    opt_trade_eth.dir = "../log";
    opt_trade_eth.is_async = true;
    opt_trade_eth.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_trade_eth.max_files = 50;                 // up to X GB total

    utils::Logger::Options opt_log;
    opt_log.name = "log";
    opt_log.file = "log.txt";
    opt_log.dir = "../log";
    opt_log.is_async = true;
    opt_log.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_log.max_files = 50;                 // up to X GB total

    utils::GlobalLog::ins().init(opt_log);
    agg_logger_xrp.init(opt_agg_xrp);
    agg_logger_eth.init(opt_agg_eth);
    trade_logger_xrp.init(opt_trade_xrp);
    trade_logger_eth.init(opt_trade_eth);

    // aggtrade
    std::string addr_agg = "wss://fstream.binance.com/stream?streams=xrpusdt@aggTrade";
    std::string subs_msg_agg = R"({"method":"SUBSCRIBE","params":["xrpusdt@aggTrade"],"id":11})";
    std::string cancel_subs_msg_agg = R"({"method":"UNSUBSCRIBE","params":["xrpusdt@aggTrade"],"id":11})";

    std::string addr_agg1 = "wss://fstream.binance.com/stream?streams=ethusdt@aggTrade";
    std::string subs_msg_agg1 = R"({"method":"SUBSCRIBE","params":["ethusdt@aggTrade"],"id":12})";
    std::string cancel_subs_msg_agg1 = R"({"method":"UNSUBSCRIBE","params":["ethusdt@aggTrade"],"id":12})";

    // trade
    std::string addr_agg_trade = "wss://fstream.binance.com/stream?streams=xrpusdt@trade";
    std::string subs_msg_agg_trade = R"({"method":"SUBSCRIBE","params":["xrpusdt@trade"],"id":13})";
    std::string cancel_subs_msg_agg_trade = R"({"method":"UNSUBSCRIBE","params":["xrpusdt@trade"],"id":13})";

    std::string addr_agg_trade1 = "wss://fstream.binance.com/stream?streams=ethusdt@trade";
    std::string subs_msg_agg_trade1 = R"({"method":"SUBSCRIBE","params":["ethusdt@trade"],"id":14})";
    std::string cancel_subs_msg_agg_trade1 = R"({"method":"UNSUBSCRIBE","params":["ethusdt@trade"],"id":14})";

    std::vector<std::thread> ws_threads;

    ws_threads.emplace_back(wbSocketThread, addr_agg, subs_msg_agg, cancel_subs_msg_agg);
    ws_threads.emplace_back(wbSocketThread, addr_agg1, subs_msg_agg1, cancel_subs_msg_agg1);
    ws_threads.emplace_back(wbSocketThread, addr_agg_trade, subs_msg_agg_trade, cancel_subs_msg_agg_trade);
    ws_threads.emplace_back(wbSocketThread, addr_agg_trade1, subs_msg_agg_trade1, cancel_subs_msg_agg_trade1);

    for (auto& t : ws_threads) {
        t.join();
    }

    return 0;
}