#include "logger.h"
#include "quickfix/fix44/ExecutionReport.h"
#include "quickfix/fix44/TradeCaptureReport.h"
#include "quickfix/fix44/TradeCaptureReportRequest.h"
#include "quickfix/fix44/TradeCaptureReportRequestAck.h"
#include "trading_application.h"
#include <sstream>
#include <unordered_map>

static utils::Logger logger;
unordered_map<string, int64_t> m;
static int count = 0;
class TradeCaptureReportRequest : public TradingApplication {
public:
  void onLogon(const FIX::SessionID &session_id) override {
    std::cout << "on logon and send" << std::endl;

    tradeCaptureReportRequest(session_id);
  }

  void tradeCaptureReportRequest(const FIX::SessionID &session_id) {
    std::cout << "start sending" << std::endl;
    FIX44::TradeCaptureReportRequest req;
    req.set(FIX::TradeRequestID("id111"));
    req.set(FIX::TradeRequestType(0));
    req.set(FIX::Symbol("ETH-PERPETUAL"));
    req.set(FIX::SubscriptionRequestType('1'));

    try {
      FIX::Session::sendToTarget(
          req, session_id); // send 发送当前session，sendToTarget发送指定session
    } catch (FIX::SessionNotFound &e) {
      std::cerr << "error:" << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Unknown exception caught in marketDataRequestSend"
                << std::endl;
    }
  }

  void onMessage(const FIX44::TradeCaptureReportRequestAck &message,
                 const FIX::SessionID &) override {
    std::cout << "received trade capture report request ack:" << std::endl;
    printMsg(message);
  }

  void onMessage(const FIX44::TradeCaptureReport &message,
                 const FIX::SessionID &) override {
    std::cout << "received trade capture report:" << std::endl;
    printMsg(message);
  }

  void onMessage(const FIX44::ExecutionReport &message,
                 const FIX::SessionID &) override {
    FIX::OrigClOrdID origClOrdId;
    message.get(origClOrdId);
    m[origClOrdId] = common::getTimeStampNs();
    ++count;

    if (count == 10) {
      for (auto it : msend) {
        std::stringstream ss;
        ss << "id" << it.first << ",receiving time ns:[" << it.second
           << std::endl;
        logger.info(ss.str());
      }
      logger.flush();
    }
  }
};

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "usage:" << argv[0] << "config file" << std::endl;
    return 1;
  }
  std::string config_file = argv[1];

  utils::Logger::Options opt_log;
  opt_log.name = "log";
  opt_log.file = "log.txt";
  opt_log.dir = "../log";
  opt_log.is_async = true;
  opt_log.max_size = 1024L * 1024 * 100; // 100 MB per file
  opt_log.max_files = 50;                // up to X GB total
  logger.init(opt_log);

  try {
    FIX::SessionSettings settings(config_file);
    TradeCaptureReportRequest app;
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