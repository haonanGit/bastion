#include "common.h"
// #include "logger.h"
#include "quickfix/fix44/ExecutionReport.h"
#include "quickfix/fix44/NewOrderSingle.h"
#include "trading_application.h"
#include <atomic>
#include <thread>
#include <unordered_map>

std::unordered_map<std::string, int64_t> msend, mrecv;
int num = 10;
// static utils::Logger logger;

class NewOrderSingle : public TradingApplication {
public:
  void onLogon(const FIX::SessionID &session_id) override {
    std::cout << "on logon and send" << std::endl;
    std::thread(&runInThread, this, session_id).detach();
  }

  static void runInThread(const FIX::SessionID &session_id) {
    for (int i = 0; i < 10; ++i) {
      newOrderSingle(session_id, 3800.00, i);
    }
  }
  void newOrderSingle(const FIX::SessionID &session_id, double price = 0.0,
                      int count = 0) {
    // std::cout << "start sending" << std::endl;
    std::string clordid = "test" + std::to_string(count);
    FIX44::NewOrderSingle req;
    req.set(FIX::ClOrdID(clordid));
    req.set(FIX::Side('2')); // 2 sell
    req.set(FIX::OrderQty(1));
    req.set(FIX::Price(price));
    req.set(FIX::Symbol("ETH-PERPETUAL"));
    req.setField(FIX::ExecInst("6"));
    req.set(FIX::OrdType('2'));
    req.set(FIX::TimeInForce('0'));
    req.setField(FIX::IntField(5127, 1)); // DeribitConditionTriggerMethod

    try {
      msend[clordid] = common::getTimeStampNs();
      FIX::Session::sendToTarget(
          req, session_id); // send 发送当前session，sendToTarget发送指定session
    } catch (FIX::SessionNotFound &e) {
      std::cerr << "error:" << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Unknown exception caught in marketDataRequestSend"
                << std::endl;
    }
  }

  void onMessage(const FIX44::ExecutionReport &message,
                 const FIX::SessionID &) override {
    FIX::OrigClOrdID origClOrdId;
    if (message.isSetField(origClOrdId)) {
      message.get(origClOrdId);
      mrecv[origClOrdId] = common::getTimeStampNs();
      --num;
    }

    if (num == 0) {
      long long total = 0;
      for (auto it : msend) {
        auto rtt = mrecv[it.first] - it.second;
        std::cout << "id" << it.first << ",sending time ns:[" << it.second
                  << "],receiving time ns:[" << mrecv[it.first] << "], rtt:["
                  << rtt << "]" << std::endl;
        total += rtt;
      }
      std::cout << std::fixed << std::setprecision(1) << "avg rtt ms:["
                << static_cast<double>(total) / (10 * 100000) << "]"
                << std::endl;
    }
    // std::cout << "received execution report:" << std::endl;
    // printMsg(message);
  }
};

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "usage:" << argv[0] << "config file" << std::endl;
    return 1;
  }
  std::string config_file = argv[1];

  //   utils::Logger::Options opt_log;
  //   opt_log.name = "log";
  //   opt_log.file = "log.txt";
  //   opt_log.dir = "./bus_log";
  //   opt_log.is_async = true;
  //   opt_log.max_size = 1024L * 1024 * 100; // 100 MB per file
  //   opt_log.max_files = 50;                // up to X GB total
  //   logger.init(opt_log);

  try {
    FIX::SessionSettings settings(config_file);
    NewOrderSingle app;
    FIX::FileStoreFactory store_factory(settings);
    FIX::FileLogFactory log_factory(settings);
    FIX::SocketInitiator init(app, store_factory, settings, log_factory);
    std::cout << "main start" << std::endl;
    init.start();
    while (1) {
      //
    }
    init.stop();
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception caught in main" << std::endl;
  }

  return 0;
}