#ifndef TRADING_APPLICATION_H
#define TRADING_APPLICATION_H

#include "common.h"
#include "fix_print.h"
#include "quickfix/Application.h"
#include "quickfix/Field.h"
#include "quickfix/FileLog.h"
#include "quickfix/FileStore.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/SessionSettings.h"
#include "quickfix/SocketInitiator.h"
#include "quickfix/fix44/Logon.h"
#include "quickfix/fix44/SequenceReset.h"
#include <chrono>
#include <cstring>
#include <iostream>

// #define client_id "API-KEY"
// #define application_secret "API-SECRET"
// #define test_nonce "test_deribit_testnet_incremental_00000"
// #define test_timestamp "1709791993000"
#define client_id "v06Q_1i0"
#define application_secret "oltBcF4AwCb1VpXBFmNUs-Po3_p2gblXBzBmqS0tAX4"

std::string generateTimestamp() {
  using namespace std::chrono;
  auto now = system_clock::now();
  auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
  return std::to_string(ms);
}

// 生成RawData字段值
std::string generateRawData() {
  std::string timestamp = generateTimestamp();
  std::string base64_nonce = common::GenerateRandomNonce(64);

  // std::string timestamp = test_timestamp;
  // std::string base64_nonce = common::Base64Encode(reinterpret_cast<const
  // unsigned char*>(test_nonce), strlen(test_nonce));
  std::string raw_data = timestamp + "." + base64_nonce;
  // std::cout << "generateRawData:" << raw_data << std::endl;
  return raw_data;
}

std::string generatePassword(const std::string &raw_data) {
  std::string base_signature_string = raw_data + application_secret;
  // std::cout << "base_signature_string:" << base_signature_string <<
  // std::endl;
  std::string sha256_str = common::sha256RawString((base_signature_string));
  std::string password = common::Base64Encode(
      reinterpret_cast<const unsigned char *>(sha256_str.c_str()),
      strlen(sha256_str.c_str()));
  // std::cout << "generatePassword:" << password << std::endl;
  return password;
}

class TradingApplication : public FIX::Application, public FIX::MessageCracker {
public:
  void onCreate(const FIX::SessionID &session_id) override {
    std::cout << "on create" << std::endl;
  }

  void onLogon(const FIX::SessionID &session_id) override {
    std::cout << "on logon and send" << std::endl;

    // sendSequenceReset(session_id);
    // marketDataRequestSend(session_id);
  }

  void onLogout(const FIX::SessionID &session_id) override {
    std::cout << "on logout" << std::endl;
  }

  void toAdmin(FIX::Message &message,
               const FIX::SessionID &session_id) override {
    // std::cout << "to admin message:" << std::endl;
    // printMsg(message);
    auto &header = message.getHeader();
    // logon for the first time
    if (header.getField(FIX::FIELD::MsgType) == FIX::MsgType_Logon) {
      SetlogonData(message, session_id);
      // message.setField(FIX::ResetSeqNumFlag(true));  // 重置序列号
      std::cout << "set logon message :" << message << std::endl;
    }
  }

  void toApp(FIX::Message &message, const FIX::SessionID &) override {
    try {
      //   std::cout << "to app message:" << std::endl;
      //   printMsg(message);
    } catch (const FIX::DoNotSend &e) {
      std::cerr << "message should not be sent:" << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Unknown exception caught in toApp" << std::endl;
    }
  }

  void fromAdmin(const FIX::Message &message,
                 const FIX::SessionID &session_id) override {
    try {
      //   std::cout << "from admin :" << std::endl;
      //   printMsg(message);
    } catch (const FIX::FieldNotFound &e) {
      std::cerr << "filed not found:" << e.what() << std::endl;
    } catch (const FIX::IncorrectDataFormat &e) {
      std::cerr << "incorrect data format:" << e.what() << std::endl;
    } catch (const FIX::IncorrectTagValue &e) {
      std::cerr << "incorrect tag value:" << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Unknown exception caught in fromAdmin" << std::endl;
    }
  }

  void fromApp(const FIX::Message &message,
               const FIX::SessionID &session_id) override {
    try {
      // std::cout << "from app :" << std::endl;
      // printMsg(message);
      crack(message, session_id);
    } catch (const FIX::FieldNotFound &e) {
      std::cerr << "filed not found:" << e.what() << std::endl;
    } catch (const FIX::IncorrectDataFormat &e) {
      std::cerr << "incorrect data format:" << e.what() << std::endl;
    } catch (const FIX::IncorrectTagValue &e) {
      std::cerr << "incorrect tag value:" << e.what() << std::endl;
    } catch (const FIX::UnsupportedMessageType &e) {
      std::cerr << "unsupported message type:" << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Unknown exception caught in fromApp" << std::endl;
    }
  }

  void SetlogonData(FIX::Message &message, const FIX::SessionID &session_id) {
    message.setField(FIX::HeartBtInt(1));
    std::string raw_data = generateRawData();
    message.setField(FIX::RawData(raw_data));
    message.setField(FIX::Username(client_id)); // todo
    message.setField(FIX::Password(generatePassword(raw_data)));
  }

  void sendSequenceReset(const FIX::SessionID &session_id, const int &seq = 1) {
    FIX44::SequenceReset req;
    auto expected_seq_num =
        FIX::Session::lookupSession(session_id)->getExpectedTargetNum();
    std::cout << "expected_seq_num!!!!!!!!!!!!!!:" << expected_seq_num
              << std::endl;
    req.set(FIX::NewSeqNo(seq));
    req.setField(FIX::RefSeqNum(expected_seq_num));
    req.setField(FIX::GapFillFlag(true));
    try {
      FIX::Session::sendToTarget(req, session_id);
    } catch (const FIX::SessionNotFound &e) {
      std::cerr << "Sequence Reset: Session not found: " << e.what()
                << std::endl;
    } catch (...) {
      std::cerr << "Unknown exception caught in sendSequenceReset" << std::endl;
    }
  }
  void onMessage(const FIX44::Logon &message, const FIX::SessionID &) override {
    std::cout << "received logon:" << std::endl;
    printMsg(message);
  }
};

#endif // TRADING_APPLICATION_H