#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>
#include "common.h"

using namespace std;
using json = nlohmann::json;

struct CancelInfo {
    string type;
    string source;
    string id;
    string symbol;
    int    result;
    int    usOut;
    int    usDiff;
};

struct CalculationInfo {
    double  total_trade_qty_trigger_time;  // total trade qty at same binance time
    int64_t total_nums_trigger_time;       // total nums at same binance time
    int64_t diff_price_nums_trigger_time;  // different price at same biance time
    int64_t pre_100ms_nums;
    int64_t pre_100ms_diff_price_nums;
    int64_t pre_300ms_nums;
    int64_t pre_300ms_diff_price_nums;
    int64_t pre_500ms_nums;
    int64_t pre_500ms_diff_price_nums;
};

unordered_map<string, CancelInfo> cancel_all;
vector<CancelInfo>                trade_cancel, agg_cancel;
deque<json>                       trade_all, aggtrade_all;
string                            log_symbol;
int                               total_cancel_no = 0;

string getId(const string& cur) {
    auto id_start = cur.find("cancel id") + 10;  // Trigger cancel, cancel id SOL_USDC_1;cancel id: SOL_USDC_535
    if (cur.find("cancel id:") != string::npos) {
        id_start++;
    }
    auto id_end = cur.find(",", id_start);
    auto id = cur.substr(id_start, id_end - id_start);
    return id;
}

string getType(const string& cur) {
    if (cur.find("by SWAP") != string::npos) {
        return "SWAP";
    } else if (cur.find("by USDT") != string::npos) {
        return "USDT";
    } else if (cur.find("by FDUSD") != string::npos) {
        return "FDUSD";
    } else if (cur.find("Deribit 1s") != string::npos) {
        return "Deribit 1s";
    }
    return "";
}

string getSource(const string& cur) {
    if (cur.find("Stream: trade") != string::npos) {
        return "trade";
    } else if (cur.find("Stream: aggTrade") != string::npos) {
        return "aggTrade";
    }

    return "";
}

string getSourceId(const string& cur) {
    auto start = cur.find("Trade Id") + 10;  // Agg Trade Id: 134235614
    auto end = cur.find(",", start);
    return cur.substr(start, end - start);
}

string getSymbol(const string& cur) {
    if (cur.find("Symbol") == string::npos) {
        return "";
    }
    auto start = cur.find("Symbol") + 7;  // Symbol SOLUSDT,
    auto end = cur.find(",", start);
    return cur.substr(start, end - start);
}

void setCancelReq(const string& cur) {
    CancelInfo info;
    auto       id = getId(cur);
    info.type = getType(cur);
    info.source = getSource(cur);
    info.id = getSourceId(cur);
    info.symbol = getSymbol(cur);

    cancel_all[id] = info;
}

void setCancelInfo(const string& cur) {
    auto id = getId(cur);
    if (cancel_all.count(id) == 0) {
        return;
    }

    json currentJson = json::parse(cur.substr(cur.find("{")), nullptr, false);

    auto info = cancel_all[id];
    info.result = currentJson["result"];
    info.usDiff = currentJson["usDiff"];
    info.usOut = currentJson["usOut"];
    cout << "id:" << id << "sourceid:" << info.id << endl;
    auto& v = info.source == "trade" ? trade_cancel : agg_cancel;
    v.emplace_back(info);
    if (log_symbol != info.symbol) {
        cout << "symbol matching failed!!!!!!!!!!!!!!! :" << id << endl;
    }
}

void getCalculationInfo(CalculationInfo& cal) {
    int    diff = 0;
    string pre_price;
    auto   base_time = common::convertToTimestamp(trade_all.back()["data"]["T"], common::TimeUnit::Milliseconds);

    cal.total_trade_qty_trigger_time = 0.0;
    cal.total_nums_trigger_time = 0;
    cal.diff_price_nums_trigger_time = 0;
    cal.pre_100ms_nums = 0;
    cal.pre_100ms_diff_price_nums = 0;
    cal.pre_300ms_nums = 0;
    cal.pre_300ms_diff_price_nums = 0;
    cal.pre_500ms_nums = 0;
    cal.pre_500ms_diff_price_nums = 0;

    while (!trade_all.empty()) {
        auto cur_time = common::convertToTimestamp(trade_all.front()["data"]["T"], common::TimeUnit::Milliseconds);
        if (base_time - cur_time > 500) {
            trade_all.pop_front();
        } else {
            break;
        }
    }
    for (auto it = trade_all.rbegin(); it != trade_all.rend(); ++it) {
        const auto& cur = trade_all.back();
        auto        cur_time = common::convertToTimestamp(cur["data"]["T"], common::TimeUnit::Milliseconds);
        diff = base_time - cur_time;
        if (diff == 0) {
            if (pre_price != cur["data"]["p"] || cal.total_nums_trigger_time == 0) {
                ++cal.diff_price_nums_trigger_time;
            }
            ++cal.total_nums_trigger_time;
            cal.total_trade_qty_trigger_time += stod(pre_price != cur["data"]["q"]);

        } else if (diff <= 100) {
            if (pre_price != cur["data"]["p"] || cal.pre_100ms_nums == 0) {
                ++cal.pre_100ms_diff_price_nums;
            }
            ++cal.pre_100ms_nums;
        } else if (diff <= 300) {
            if (pre_price != cur["data"]["p"] || cal.pre_300ms_nums == 0) {
                ++cal.pre_300ms_diff_price_nums;
            }
            ++cal.pre_300ms_nums;
        } else if (diff <= 500) {
            if (pre_price != cur["data"]["p"] || cal.pre_500ms_nums == 0) {
                ++cal.pre_500ms_diff_price_nums;
            }
            ++cal.pre_500ms_nums;
        }
        pre_price = cur["data"]["p"];
    }
}

void readCancellation(const string& file) {
    cout << "start readCancellation" << endl;
    ifstream infile(file);
    if (!infile.is_open()) {
        cerr << "Error opening file: " << file << endl;
        return;
    }

    string line;
    while (getline(infile, line)) {
        if (line.empty())
            continue;

        if (line.find("Trigger cancel, cancel id") != string::npos && !((getSymbol(line) != log_symbol) || (getType(line) == "Deribit 1s"))) {
            setCancelReq(line);
            ++total_cancel_no;
        } else if (line.find("Final Cancel Result") != string::npos) {
            setCancelInfo(line);
        }
    }
}

void readTradeLog(const string& file) {
    cout << "start readTradeLog" << endl;
    ifstream infile(file);
    if (!infile.is_open()) {
        cerr << "Error opening file: " << file << endl;
        return;
    }

    ofstream tradeFile("./trade.csv", ios::trunc);
    tradeFile << "result,"
              << "trigger symbol,"
              << "trigger qty,"
              << "trigger trade id,"
              << "trigger trade time by binance time,"
              << "total trade qty at same binance time,"
              << "total trade nums at same binance time,"
              << "different price at same biance time,"
              << "pre 100ms nums,"
              << "pre 100ms diff price nums,"
              << "pre 300ms nums,"
              << "pre 300ms diff price nums,"
              << "pre 500ms nums,"
              << "pre 500ms diff price nums,";
    tradeFile << "cancel type,"
              << "processing time us";

    size_t idx = 0;
    string line;
    cout << "trade cancel size:" << trade_cancel.size() << endl;
    while (getline(infile, line) && idx < trade_cancel.size()) {
        if (line.empty())
            continue;

        line = line.substr(line.find("{"));
        json currentJson = json::parse(line, nullptr, false);
        trade_all.emplace_back(currentJson);

        cout << "idx:" << idx << ",sourceid:" << trade_cancel[idx].id << endl;

        if (trade_cancel[idx].id == to_string(currentJson["data"]["t"])) {
            cout << "!!!!!!!!!!!!!" << endl;
            CalculationInfo cal;
            getCalculationInfo(trade_cancel[idx], cal);
            cout << " match trade cancel fail,id:[" << currentJson["data"]["t"] << "]" << endl;
            tradeFile << trade_cancel[idx].result << ",";
            tradeFile << trade_cancel[idx].symbol << ",";  // symbol
            tradeFile << currentJson["data"]["q"] << ",";  // trigger qty
            tradeFile << currentJson["data"]["t"] << ",";  // trigger trade id
            tradeFile << currentJson["data"]["T"] << ",";  // trigger trade time at binance time
            // cal info
            tradeFile << cal.total_trade_qty_trigger_time << ",";  // total trade nums at same binance time
            tradeFile << cal.total_nums_trigger_time << ",";       // total trade nums at same binance time
            tradeFile << cal.diff_price_nums_trigger_time << ",";  // different price at same biance time
            tradeFile << cal.pre_100ms_nums << ",";                // pre 100ms nums
            tradeFile << cal.pre_100ms_diff_price_nums << ",";     // pre 100ms diff price nums
            tradeFile << cal.pre_300ms_nums << ",";
            tradeFile << cal.pre_300ms_diff_price_nums << ",";
            tradeFile << cal.pre_500ms_nums << ",";
            tradeFile << cal.pre_500ms_diff_price_nums << ",";

            tradeFile << trade_cancel[idx].type << ",";
            tradeFile << trade_cancel[idx].usDiff << ",";
            tradeFile << trade_cancel[idx].usOut;
            ++idx;
        }
    }
    tradeFile.close();
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " <cancel_file_path> <log_file_path> <trade/agg> <symbol>" << endl;
        return 1;
    }

    string cancelFile = argv[1];
    string logFile = argv[2];
    string type = argv[3];
    log_symbol = argv[4];

    cout << "cancelFile:" << argv[1] << ",logfile:" << argv[2] << ",type:" << argv[3] << ",log_symbol" << argv[4] << endl;

    readCancellation(cancelFile);
    if (type == "trade") {
        readTradeLog(logFile);
    }

    return 0;
}