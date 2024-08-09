
#include "get_open_orders_by_instrument.h"
#include "common.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "vector"
#include <fstream>

#define client_id "v06Q_1i0"
#define application_secret "oltBcF4AwCb1VpXBFmNUs-Po3_p2gblXBzBmqS0tAX4"

int count = 0;
int threhold = INT_MAX;
static utils::Logger agg_logger_ethfdusd;
static utils::Logger agg_logger_ethusdt;
static utils::Logger trade_logger_ethfdusd;
static utils::Logger trade_logger_ethusdt;

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

void GetOpenOrdersByInstrument::onAuth(websocketpp::connection_hdl hdl) {
  printf("send auth\n");
  auto timestamp = common::getTimeStampNs() / 1000000;
  std::string nonce = common::GenerateRandomNonce(8);
  std::string data = "";
  std::string sign = std::to_string(timestamp) + "\n" + nonce + "\n" + data;
  std::string signature =
      common::hmac_sha256HexString(application_secret, sign);

  nlohmann::json msg;
  msg["jsonrpc"] = "2.0";
  msg["id"] = 8748;
  msg["method"] = "public/auth";
  msg["params"] = {{"grant_type", "client_signature"},
                   {"client_id", client_id},
                   {"timestamp", timestamp},
                   {"signature", signature},
                   {"nonce", nonce},
                   {"data", data}};

  send(hdl, msg.dump());
}

void GetOpenOrdersByInstrument::onMessage(websocketpp::connection_hdl hdl,
                                          app_tls_client::message_ptr msg) {
  const auto &recv_tm = common::getTimeStampNs();
  // const auto&    ret_msg = msg->get_payload();
  std::cout << "on message" << std::endl;
  nlohmann::json jsonObject = nlohmann::json::parse(msg->get_payload());
  std::cout << jsonObject.dump() << std::endl;
  //   convertTimestampToDate(jsonObject);

  //   jsonObject["received_t"] =
  //       common::timestampToDate(recv_tm, common::TimeUnit::Nanoseconds);
  //   const auto &ret_msg = jsonObject.dump();
  //   // ++count;

  //   utils::Logger logger;
  //   if (jsonObject["data"]["e"] == "trade") {
  //     logger = jsonObject["data"]["s"] == "ETHFDUSD" ? trade_logger_ethfdusd
  //                                                    : trade_logger_ethusdt;
  //   } else {
  //     logger = jsonObject["data"]["s"] == "ETHFDUSD" ? agg_logger_ethfdusd
  //                                                    : agg_logger_ethusdt;
  //   }
  //   // auto& logger = jsonObject["data"]["e"] == "trade" ? trade_logger :
  //   // agg_logger; auto  mid_tm = common::getTimeStampNs();
  //   logger.info(ret_msg);
  //   // auto        end_tm = common::getTimeStampNs();
  //   // std::string info = "json:[" + std::to_string(mid_tm - recv_tm) + "]ns
  //   " +
  //   // "log:[" + std::to_string(end_tm - mid_tm) + "] " + "total:[" +
  //   //                    std::to_string(end_tm - recv_tm) + "]";
  //   // LOG_INFO(info);
}

} // namespace app

void wbSocketThread(const std::string_view &addr,
                    const std::string_view &sbus_msg,
                    const std::string_view &cancel_subs_msg) {
  std::cout << "addr : " << addr << std::endl;
  app::GetOpenOrdersByInstrument client(addr, sbus_msg, cancel_subs_msg);
  client.start();
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    std::string s_threhold = argv[1];
    threhold = std::stoi(s_threhold);
  }

  utils::Logger::Options opt_log;
  opt_log.name = "log";
  opt_log.file = "log.txt";
  opt_log.dir = "../log";
  opt_log.is_async = true;
  opt_log.max_size = 1024L * 1024 * 100; // 100 MB per file
  opt_log.max_files = 50;                // up to X GB total

  utils::GlobalLog::ins().init(opt_log);

  // aggtrade
  std::string addr_agg = "wss://www.deribit.com/ws/api/v2";
  std::string subs_msg_agg =
      R"({"jsonrpc":"2.0","id":8442,"method":"private/get_open_orders_by_instrument","params":{"instrument_name":"ETH-PERPETUAL"}})";
  std::string cancel_subs_msg_agg =
      R"({"method":"UNSUBSCRIBE","params":["ethfdusd@aggTrade"],"id":11})";

  std::vector<std::thread> ws_threads;

  ws_threads.emplace_back(wbSocketThread, addr_agg, subs_msg_agg,
                          cancel_subs_msg_agg);

  for (auto &t : ws_threads) {
    t.join();
  }

  return 0;
}