#include "quickfix/fix44/MarketDataIncrementalRefresh.h"
#include "quickfix/fix44/MarketDataRequest.h"
#include "quickfix/fix44/MarketDataSnapshotFullRefresh.h"
#include "trading_application.h"

class MarketDataRequest : public TradingApplication {
public:
    void onLogon(const FIX::SessionID& session_id) override {
        std::cout << "on logon and send" << std::endl;

        // sendSequenceReset(session_id);

        marketDataRequestSend(session_id);
    }

    void marketDataRequestSend(const FIX::SessionID& session_id) {
        std::cout << "start sending" << std::endl;
        FIX44::MarketDataRequest req;
        req.set(FIX::MDReqID("albert002"));
        req.set(FIX::SubscriptionRequestType('1'));
        req.set(FIX::MarketDepth(1));
        req.set(FIX::MDUpdateType(0));
        req.setField(FIX::BoolField(9011, false));  // DeribitSkipBlockTrades
        req.setField(FIX::BoolField(9012, true));   // DeribitShowBlockTradeId
        req.setField(FIX::IntField(100007, 20));    // DeribitTradeAmount
        req.setField(FIX::IntField(100008, 2000));  // DeribitSinceTimestamp

        // MDReqGrp group
        FIX44::MarketDataRequest::NoMDEntryTypes no_md_entry_types;
        no_md_entry_types.set(FIX::MDEntryType(FIX::MDEntryType_BID));
        req.addGroup(no_md_entry_types);
        no_md_entry_types.set(FIX::MDEntryType(FIX::MDEntryType_OFFER));
        req.addGroup(no_md_entry_types);
        no_md_entry_types.set(FIX::MDEntryType(FIX::MDEntryType_TRADE));
        req.addGroup(no_md_entry_types);

        //  InstrmtMDReqGrp group
        FIX44::MarketDataRequest::NoRelatedSym no_related_sym;
        no_related_sym.set(FIX::Symbol("BTC-PERPETUAL"));
        req.addGroup(no_related_sym);

        try {
            // FIX::Session::sendToTarget(req, session_id);  // send 发送当前session，sendToTarget发送指定session
            for (int i = 0; i < 50; ++i) {
                FIX::Session::sendToTarget(req, session_id);  // send 发送当前session，sendToTarget发送指定session
            }
        } catch (FIX::SessionNotFound& e) {
            std::cerr << "error:" << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception caught in marketDataRequestSend" << std::endl;
        }
    }

    void onMessage(const FIX44::MarketDataSnapshotFullRefresh& message, const FIX::SessionID&) override {
        std::cout << "received market data snapshot full refresh:" << std::endl;
        printMsg(message);
    }
    void onMessage(const FIX44::MarketDataIncrementalRefresh& message, const FIX::SessionID&) override {
        std::cout << "received market data incremental refresh:" << std::endl;
        printMsg(message);
    }
};

int main(int argc, char** argv) {
    // std::cout << "test base64:" << common::Base64Encode(reinterpret_cast<const unsigned char*>("compute sha1"), strlen("compute sha1")) <<
    // std::endl; std::cout << "test albert sha256:" << common::sha256("albert") << std::endl;

    if (argc < 2) {
        std::cout << "usage:" << argv[0] << "config file" << std::endl;
        return 1;
    }
    std::string config_file = argv[1];
    try {
        FIX::SessionSettings  settings(config_file);
        MarketDataRequest     app;
        FIX::FileStoreFactory store_factory(settings);
        FIX::FileLogFactory   log_factory(settings);
        FIX::SocketInitiator  init(app, store_factory, settings, log_factory);
        std::cout << "main start" << std::endl;
        init.start();
        while (1) {
            //
        }
        init.stop();
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught in main" << std::endl;
    }

    return 0;
}