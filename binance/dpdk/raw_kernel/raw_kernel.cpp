
#include "raw_kernel.h"
#include <chrono>
#include <fstream>
#include "common.h"
#include "nlohmann/json.hpp"
#include "vector"

int count = 0;
int threhold = 200;

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

void RawKernelClient::onMessage(websocketpp::connection_hdl hdl, app_client::message_ptr msg) {
    std::string    ret_msg = msg->get_payload();
    std::string    json_msg;
    nlohmann::json jsonObject = nlohmann::json::parse(ret_msg);
    convertTimestampToDate(jsonObject);

    jsonObject["count"] = count;
    ret_msg = jsonObject.dump();
    ++count;

    std::ofstream outfile;
    outfile.open("result.txt", std::ios_base::app);
    if (outfile.is_open()) {
        outfile << ret_msg << std::endl;
        outfile.close();
        // std::cout << "Message written to result.txt successfully." << std::endl;
    } else {
        std::cerr << "Error opening file result.txt for writing." << std::endl;
    }

    // std::ofstream outfilejson;
    // outfilejson.open("json.txt", std::ios_base::app);
    // if (outfilejson.is_open()) {
    //     outfilejson << json_msg << std::endl;
    //     outfilejson.close();
    //     // std::cout << "Message written to json.txt successfully." << std::endl;
    // } else {
    //     std::cerr << "Error opening file json.txt for writing." << std::endl;
    // }
    if (count % 1000 == 1) {
        std::cout << "count : " << count << std::endl;
    }

    if (count == threhold) {
        onCancelSubscripton(hdl);
        stop();
    }
}

void wbSocketThread(const std::string_view& addr, const std::string_view& sbus_msg, const std::string_view& cancel_subs_msg) {
    RawKernelClient client(addr, sbus_msg, cancel_subs_msg);
    client.start();
}

}  // namespace app

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string s_threhold = argv[1];
        threhold = std::stoi(s_threhold);
    }
    std::cout << "fetch records: " << threhold << std::endl;
    // std::string addr =
    // "wss://fstream.binance.com/stream?streams=btcusdt@aggTrade/btcusdt@markPrice/btcusdt@depth/!markPrice@arr/btcusdt@miniTicker/"
    //                    "!bookTicker/!miniTicker@arr/btcusdt@bookTicker/!ticker@arr/btcusdt@ticker/btcusdt@forceOrder/btcusdt@depth5/btcusdt@depth/"
    //                    "btcusdt@compositeIndex/!assetIndex@arr";
    // std::string subs_msg =
    //     R"({"method":"SUBSCRIBE","params":["btcusdt@aggTrade","btcusdt@markPrice","btcusdt@depth","!markPrice@arr","btcusdt@miniTicker","!ticker@arr","btcusdt@ticker","!miniTicker@arr","btcusdt@bookTicker","!bookTicker","btcusdt@forceOrder","!forceOrder@arr","btcusdt@depth5","btcusdt@depth","btcusdt@compositeIndex","!assetIndex@arr"],"id":1})";
    // std::string cancel_subs_msg =
    //     R"({"method":"UNSUBSCRIBE","params":["btcusdt@aggTrade","btcusdt@markPrice","btcusdt@depth","!markPrice@arr","btcusdt@miniTicker","!ticker@arr","btcusdt@ticker","!miniTicker@arr","btcusdt@bookTicker","!bookTicker","btcusdt@forceOrder","!forceOrder@arr","btcusdt@depth5","btcusdt@depth","btcusdt@compositeIndex","!assetIndex@arr"],"id":1})";

    // std::string addr = "wss://fstream.binance.com/stream?streams=xrpusdt@trade/xrpusdt@aggTrade/xrpusdt@bookTicker/xrpusdt@depth5";
    // std::string subs_msg = R"({"method":"SUBSCRIBE","params":["xrpusdt@trade","xrpusdt@aggTrade","xrpusdt@bookTicker","xrpusdt@depth5"],"id":1})";
    // std::string cancel_subs_msg =
    //     R"({"method":"UNSUBSCRIBE","params":["xrpusdt@trade","xrpusdt@aggTrade","xrpusdt@bookTicker","xrpusdt@depth5"],"id":1})";

    std::string addr = "wss://fstream.binance.com/stream?streams=xrpusdt@trade/xrpusdt@aggTrade";
    std::string subs_msg = R"({"method":"SUBSCRIBE","params":["xrpusdt@trade","xrpusdt@aggTrade"],"id":1})";
    std::string cancel_subs_msg = R"({"method":"UNSUBSCRIBE","params":["xrpusdt@trade","xrpusdt@aggTrade"],"id":1})";

    // std::string              addr = "wss://fstream.binance.com/stream?streams=xrpusdt@trade";
    // std::string              subs_msg = R"({"method":"SUBSCRIBE","params":["xrpusdt@trade"],"id":1})";
    // std::string              cancel_subs_msg = R"({"method":"UNSUBSCRIBE","params":["xrpusdt@trade"],"id":1})";
    int                      thread_nums = 1;
    std::vector<std::thread> ws_threads;

    std::ofstream outfile("result.txt", std::ios_base::trunc);
    if (outfile.is_open()) {
        outfile.close();
        std::cout << "Empty result.txt successfully." << std::endl;
    } else {
        std::cerr << "Error opening file result.txt for writing." << std::endl;
    }

    std::ofstream outfilejson("json.txt", std::ios_base::trunc);
    if (outfilejson.is_open()) {
        outfilejson.close();
        std::cout << "Empty json.txt successfully." << std::endl;
    } else {
        std::cerr << "Error opening file json.txt for writing." << std::endl;
    }

    for (int i = 0; i < thread_nums; ++i) {
        ws_threads.emplace_back(app::wbSocketThread, addr, subs_msg, cancel_subs_msg);
    }

    for (auto& t : ws_threads) {
        t.join();
    }

    return 0;
}