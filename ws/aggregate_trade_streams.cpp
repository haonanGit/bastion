
#include "aggregate_trade_streams.h"
#include "common.h"
#include "nlohmann/json.hpp"
#include "vector"
#include <chrono>
#include <fstream>

int count = 0;
int threhold = INT_MAX;
// std::ofstream outfile("result.txt", std::ios_base::trunc);

namespace app {

void convertTimestampToDate(nlohmann::json &jsonObject) {
  // Aggregate Trade Streams
  if (jsonObject.contains("data") && !jsonObject["data"].is_null() &&
      jsonObject["data"].contains("E") && !jsonObject["data"]["E"].is_null() &&
      jsonObject["data"].contains("T") && !jsonObject["data"]["T"].is_null()) {
    long long E = jsonObject["data"]["E"];
    long long T = jsonObject["data"]["T"];
    std::string E_date =
        common::timestampToDate(E, common::TimeUnit::Milliseconds);
    std::string T_date =
        common::timestampToDate(T, common::TimeUnit::Milliseconds);
    jsonObject["data"]["E"] = E_date;
    jsonObject["data"]["T"] = T_date;
  }
}

void AggregateTradeStreamsClient::onMessage(websocketpp::connection_hdl hdl,
                                            app_tls_client::message_ptr msg) {
  std::string ret_msg = msg->get_payload();
  std::string json_msg;
  nlohmann::json jsonObject = nlohmann::json::parse(ret_msg);
  convertTimestampToDate(jsonObject);

  const auto &tm = common::getNow();
  jsonObject["received_t"] = timestampToDate(tm, common::TimeUnit::Nanoseconds);
  ret_msg = jsonObject.dump();

  result_logger.info();
  // if (outfile.is_open()) {
  //     outfile << ret_msg << std::endl;

  // } else {
  //     std::cerr << "Error opening file result.txt for writing." << std::endl;
  // }
}

} // namespace app

static utils::Logger result_logger;

void wbSocketThread(const std::string_view &addr,
                    const std::string_view &sbus_msg,
                    const std::string_view &cancel_subs_msg) {
  std::cout << "addr : " << addr << std::endl;
  AggregateTradeStreamsClient client(addr, sbus_msg, cancel_subs_msg);
  client.start();
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    std::string s_threhold = argv[1];
    threhold = std::stoi(s_threhold);
  }

  // init log
  utils::Logger::Options opt_result;
  opt_result.name = "result";
  opt_result.file = "result.txt";
  opt_result.dir = "./log";
  opt_result.is_async = true;
  opt_result.max_size = 1024L * 1024 * 100; // 100 MB per file
  opt_result.max_files = 100;               // up to 10 GB total

  utils::Logger::Options opt_log;
  opt_result.name = "log";
  opt_result.file = "log.txt";
  opt_result.dir = "./log";
  opt_result.is_async = true;
  opt_result.max_size = 1024L * 1024 * 100; // 100 MB per file
  opt_result.max_files = 100;               // up to 10 GB total

  utils::GlobalLog::ins().init(opt_log);
  utils::GlobalLog::ins().start();
  result_logger.init(opt_result);
  result_logger.start();

  // init url
  std::string addr = "wss://fstream.binance.com/stream?streams=xrpusdt@trade/"
                     "xrpusdt@aggTrade/ethusdt@aggTrade";
  std::string subs_msg =
      R"({"method":"SUBSCRIBE","params":["xrpusdt@trade","xrpusdt@aggTrade"],"id":1})";
  std::string cancel_subs_msg =
      R"({"method":"UNSUBSCRIBE","params":["xrpusdt@trade","xrpusdt@aggTrade"],"id":1})";

  int thread_nums = 1;
  std::vector<std::thread> ws_threads;

  // if (outfile.is_open()) {
  //     std::cout << "Empty result.txt successfully." << std::endl;
  // } else {
  //     std::cerr << "Error opening file result.txt for writing." << std::endl;
  // }

  for (int i = 0; i < thread_nums; ++i) {
    ws_threads.emplace_back(wbSocketThread, addr, subs_msg, cancel_subs_msg);
  }

  for (auto &t : ws_threads) {
    t.join();
  }

  return 0;
}