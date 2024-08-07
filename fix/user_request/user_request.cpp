#include "quickfix/fix44/UserRequest.h"
#include "quickfix/fix44/UserResponse.h"
#include "trading_application.h"

class UserRequest : public TradingApplication {
public:
    void onLogon(const FIX::SessionID& session_id) override {
        std::cout << "on logon and send" << std::endl;

        userRequest(session_id);
    }

    void userRequest(const FIX::SessionID& session_id) {
        std::cout << "start sending" << std::endl;
        FIX44::UserRequest req;
        req.set(FIX::UserRequestID("test001"));
        req.set(FIX::UserRequestType(4));
        req.set(FIX::Username("v06Q_1i0"));
        req.setField(FIX::Currency("ETH"));

        try {
            FIX::Session::sendToTarget(req, session_id);  // send 发送当前session，sendToTarget发送指定session
        } catch (FIX::SessionNotFound& e) {
            std::cerr << "error:" << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception caught in marketDataRequestSend" << std::endl;
        }
    }

    void onMessage(const FIX44::UserResponse& message, const FIX::SessionID&) override {
        std::cout << "received user response:" << std::endl;
        printMsg(message);
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "usage:" << argv[0] << "config file" << std::endl;
        return 1;
    }
    std::string config_file = argv[1];
    try {
        FIX::SessionSettings  settings(config_file);
        UserRequest           app;
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