#include "quickfix/fix44/ExecutionReport.h"
#include "quickfix/fix44/NewOrderSingle.h"
#include "trading_application.h"

class NewOrderSingle : public TradingApplication {
public:
  void onLogon(const FIX::SessionID &session_id) override {
    std::cout << "on logon and send" << std::endl;

    newOrderSingle(session_id, 3800.00);
  }

  void newOrderSingle(const FIX::SessionID &session_id, double price = 0.0) {
    std::cout << "start sending" << std::endl;
    FIX44::NewOrderSingle req;
    req.set(FIX::ClOrdID("test001"));
    req.set(FIX::Side('2')); // 2 sell
    req.set(FIX::OrderQty(0.001));
    req.set(FIX::Price(price));
    req.set(FIX::Symbol("ETH-PERPETUAL"));
    req.setField(FIX::ExecInst("6"));
    req.set(FIX::OrdType('2'));
    req.set(FIX::TimeInForce('0'));
    req.setField(FIX::IntField(5127, 1)); // DeribitConditionTriggerMethod

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

  void onMessage(const FIX44::ExecutionReport &message,
                 const FIX::SessionID &) override {
    std::cout << "received execution report:" << std::endl;
    printMsg(message);
  }
};

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "usage:" << argv[0] << "config file" << std::endl;
    return 1;
  }
  std::string config_file = argv[1];
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