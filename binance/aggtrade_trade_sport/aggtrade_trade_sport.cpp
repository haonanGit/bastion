
#include <fstream>
#include "aggregate_trade_streams.h"
#include "common.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "vector"

int                  count = 0;
int                  threhold = INT_MAX;
static utils::Logger agg_logger_ethfdusd;
static utils::Logger agg_logger_ethusdt;
static utils::Logger trade_logger_ethfdusd;
static utils::Logger trade_logger_ethusdt;

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
        logger = jsonObject["data"]["s"] == "ETHFDUSD" ? trade_logger_ethfdusd : trade_logger_ethusdt;
    } else {
        logger = jsonObject["data"]["s"] == "ETHFDUSD" ? agg_logger_ethfdusd : agg_logger_ethusdt;
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
    utils::Logger::Options opt_agg_ethfdusd;
    opt_agg_ethfdusd.name = "agg_ethfdusd";
    opt_agg_ethfdusd.file = "agg_ethfdusd.txt";
    opt_agg_ethfdusd.dir = "../log";
    opt_agg_ethfdusd.is_async = true;
    opt_agg_ethfdusd.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_agg_ethfdusd.max_files = 50;                 // up to X GB total

    utils::Logger::Options opt_agg_ethusdt;
    opt_agg_ethusdt.name = "agg_ethusdt";
    opt_agg_ethusdt.file = "agg_ethusdt.txt";
    opt_agg_ethusdt.dir = "../log";
    opt_agg_ethusdt.is_async = true;
    opt_agg_ethusdt.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_agg_ethusdt.max_files = 50;                 // up to X GB total

    utils::Logger::Options opt_trade_ethfdusd;
    opt_trade_ethfdusd.name = "trade_ethfdusd";
    opt_trade_ethfdusd.file = "trade_ethfdusd.txt";
    opt_trade_ethfdusd.dir = "../log";
    opt_trade_ethfdusd.is_async = true;
    opt_trade_ethfdusd.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_trade_ethfdusd.max_files = 50;                 // up to X GB total

    utils::Logger::Options opt_trade_ethusdt;
    opt_trade_ethusdt.name = "trade_ethusdt";
    opt_trade_ethusdt.file = "trade_ethusdt.txt";
    opt_trade_ethusdt.dir = "../log";
    opt_trade_ethusdt.is_async = true;
    opt_trade_ethusdt.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_trade_ethusdt.max_files = 50;                 // up to X GB total

    utils::Logger::Options opt_log;
    opt_log.name = "log";
    opt_log.file = "log.txt";
    opt_log.dir = "../log";
    opt_log.is_async = true;
    opt_log.max_size = 1024L * 1024 * 100;  // 100 MB per file
    opt_log.max_files = 50;                 // up to X GB total

    utils::GlobalLog::ins().init(opt_log);
    agg_logger_ethfdusd.init(opt_agg_ethfdusd);
    agg_logger_ethusdt.init(opt_agg_ethusdt);
    trade_logger_ethfdusd.init(opt_trade_ethfdusd);
    trade_logger_ethusdt.init(opt_trade_ethusdt);

    // aggtrade
    std::string addr_agg = "wss://stream.binance.com/stream?streams=ethfdusd@aggTrade";
    std::string subs_msg_agg = R"({"method":"SUBSCRIBE","params":["ethfdusd@aggTrade"],"id":11})";
    std::string cancel_subs_msg_agg = R"({"method":"UNSUBSCRIBE","params":["ethfdusd@aggTrade"],"id":11})";

    std::string addr_agg1 = "wss://stream.binance.com/stream?streams=ethusdt@aggTrade";
    std::string subs_msg_agg1 = R"({"method":"SUBSCRIBE","params":["ethusdt@aggTrade"],"id":12})";
    std::string cancel_subs_msg_agg1 = R"({"method":"UNSUBSCRIBE","params":["ethusdt@aggTrade"],"id":12})";

    // trade
    std::string addr_trade = "wss://stream.binance.com/stream?streams=ethfdusd@trade";
    std::string subs_msg_trade = R"({"method":"SUBSCRIBE","params":["ethfdusd@trade"],"id":13})";
    std::string cancel_subs_msg_trade = R"({"method":"UNSUBSCRIBE","params":["ethfdusd@trade"],"id":13})";

    std::string addr_trade1 = "wss://stream.binance.com/stream?streams=ethusdt@trade";
    std::string subs_msg_trade1 = R"({"method":"SUBSCRIBE","params":["ethusdt@trade"],"id":14})";
    std::string cancel_subs_msg_trade1 = R"({"method":"UNSUBSCRIBE","params":["ethusdt@trade"],"id":14})";

    std::vector<std::thread> ws_threads;

    ws_threads.emplace_back(wbSocketThread, addr_agg, subs_msg_agg, cancel_subs_msg_agg);
    ws_threads.emplace_back(wbSocketThread, addr_agg1, subs_msg_agg1, cancel_subs_msg_agg1);
    ws_threads.emplace_back(wbSocketThread, addr_trade, subs_msg_trade, cancel_subs_msg_trade);
    ws_threads.emplace_back(wbSocketThread, addr_trade1, subs_msg_trade1, cancel_subs_msg_trade1);

    for (auto& t : ws_threads) {
        t.join();
    }

    return 0;
}