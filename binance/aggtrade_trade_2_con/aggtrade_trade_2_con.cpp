
#include <fstream>
#include "aggregate_trade_streams.h"
#include "common.h"
#include "nlohmann/json.hpp"
#include "vector"

int count = 0;
int threhold = INT_MAX;

std::ofstream aggfile("aggfile.txt", std::ios_base::trunc);
std::ofstream tradefile("tradefile.txt", std::ios_base::trunc);
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
    // ++count;

    auto& outfile = jsonObject["data"]["e"] == "trade" ? tradefile : aggfile;

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
    std::cout << "fetch records: " << threhold << std::endl;

    std::string addr_agg = "wss://fstream.binance.com/stream?streams=xrpusdt@aggTrade";
    std::string subs_msg_agg = R"({"method":"SUBSCRIBE","params":["xrpusdt@aggTrade"],"id":1})";
    std::string cancel_subs_msg_agg = R"({"method":"UNSUBSCRIBE","params":["xrpusdt@aggTrade"],"id":1})";

    std::string addr_agg_trade = "wss://fstream.binance.com/stream?streams=xrpusdt@trade";
    std::string subs_msg_agg_trade = R"({"method":"SUBSCRIBE","params":["xrpusdt@trade"],"id":2})";
    std::string cancel_subs_msg_agg_trade = R"({"method":"UNSUBSCRIBE","params":["xrpusdt@trade"],"id":2})";

    std::vector<std::thread> ws_threads;

    if (aggfile.is_open()) {
        std::cout << "Empty aggfile.txt successfully." << std::endl;
    } else {
        std::cerr << "Error opening file aggfile.txt for writing." << std::endl;
    }

    if (tradefile.is_open()) {
        std::cout << "Empty tradefile.txt successfully." << std::endl;
    } else {
        std::cerr << "Error opening file tradefile.txt for writing." << std::endl;
    }

    ws_threads.emplace_back(app::wbSocketThread, addr_agg, subs_msg_agg, cancel_subs_msg_agg);
    ws_threads.emplace_back(app::wbSocketThread, addr_agg_trade, subs_msg_agg_trade, cancel_subs_msg_agg_trade);

    for (auto& t : ws_threads) {
        t.join();
    }

    return 0;
}