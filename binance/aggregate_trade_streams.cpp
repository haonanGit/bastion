
#include "aggregate_trade_streams.h"
#include <chrono>
#include <fstream>
#include "common.h"
#include "nlohmann/json.hpp"
#include "vector"

int           count = 0;
int           threhold = INT_MAX;
std::ofstream outfile("result.txt", std::ios_base::trunc);

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

void AggregateTradeStreamsClient::onMessage(websocketpp::connection_hdl hdl, app_client::message_ptr msg) {
    std::string    ret_msg = msg->get_payload();
    std::string    json_msg;
    nlohmann::json jsonObject = nlohmann::json::parse(ret_msg);
    convertTimestampToDate(jsonObject);

    const auto& tm = common::getNow();
    jsonObject["received_t"] = timestampToDate(tm, common::TimeUnit::Nanoseconds);
    ret_msg = jsonObject.dump();

    if (outfile.is_open()) {
        outfile << ret_msg << std::endl;

    } else {
        std::cerr << "Error opening file result.txt for writing." << std::endl;
    }
}

void wbSocketThread(const std::string_view& addr, const std::string_view& sbus_msg, const std::string_view& cancel_subs_msg) {
    std::cout << "addr : " << addr << std::endl;
    AggregateTradeStreamsClient client(addr, sbus_msg, cancel_subs_msg);
    client.start();
}

}  // namespace app

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string s_threhold = argv[1];
        threhold = std::stoi(s_threhold);
    }

    std::string addr = "wss://fstream.binance.com/stream?streams=xrpusdt@trade/xrpusdt@aggTrade";
    std::string subs_msg = R"({"method":"SUBSCRIBE","params":["xrpusdt@trade","xrpusdt@aggTrade"],"id":1})";
    std::string cancel_subs_msg = R"({"method":"UNSUBSCRIBE","params":["xrpusdt@trade","xrpusdt@aggTrade"],"id":1})";

    int                      thread_nums = 1;
    std::vector<std::thread> ws_threads;

    if (outfile.is_open()) {
        std::cout << "Empty result.txt successfully." << std::endl;
    } else {
        std::cerr << "Error opening file result.txt for writing." << std::endl;
    }

    for (int i = 0; i < thread_nums; ++i) {
        ws_threads.emplace_back(app::wbSocketThread, addr, subs_msg, cancel_subs_msg);
    }

    for (auto& t : ws_threads) {
        t.join();
    }

    return 0;
}