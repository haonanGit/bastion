#include <chrono>
#include <ctime>
#include <filesystem>  // C++17标准库文件系统支持
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "common.h"

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;

struct CancelInfo {
    string    type;
    string    source;
    string    id;
    string    symbol;
    string    logTime;
    int       result;
    long long usIn;
    long long usOut;
    long long usDiff;
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
vector<CancelInfo>                trade_cancel, agg_cancel, within1s;
deque<json>                       trade_all, aggtrade_all;
string                            log_symbol;
string                            trigger_type;
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
        return "deribit1s";
    }
    return "";
}

string getSource(const string& cur) {
    if (cur.find("Stream: trade") != string::npos) {
        return "trade";
    } else if (cur.find("Stream: aggTrade") != string::npos) {
        return "aggtrade";
    }

    return "deribit";
}

string getSourceId(const string& cur) {
    if (cur.find("Trade Id") == string::npos) {
        return "";
    }
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

string getLogTime(const string& cur) {
    auto start = cur.find("INFO") + 6;  // INFO  2024-07-29 04:28:47,236 [
    auto end = cur.find(" [");
    return cur.substr(start, end - start);
}

void setCancelReq(const string& cur, const string& file) {
    CancelInfo info;
    auto       id = getId(cur) + file;
    info.type = getType(cur);
    info.source = getSource(cur);
    info.id = getSourceId(cur);
    info.symbol = getSymbol(cur);
    info.logTime = getLogTime(cur);

    cancel_all[id] = info;
}

void setCancelInfo(const string& cur, const string& file) {
    auto id = getId(cur) + file;
    if (cancel_all.count(id) == 0) {
        return;
    }

    json currentJson = json::parse(cur.substr(cur.find("{")), nullptr, false);

    auto info = cancel_all[id];
    info.result = currentJson["result"];
    info.usDiff = currentJson["usDiff"];
    info.usIn = currentJson["usIn"];
    info.usOut = currentJson["usOut"];

    if (info.source == "trade") {
        trade_cancel.emplace_back(info);
    } else if (info.source == "aggtrade") {
        agg_cancel.emplace_back(info);
    }
    if (info.source == "deribit") {
        within1s.emplace_back(info);
    }

    if (log_symbol != info.symbol && !info.symbol.empty()) {
        cout << "symbol matching failed!!!!!!!!!!!!!!! :" << id << endl;
        exit(1);
    }
}

void getCalculationInfo(CalculationInfo& cal, deque<json>& trade_que) {
    int    diff = 0;
    string pre_price;
    auto   base_time = common::convertToTimestamp(trade_que.back()["data"]["T"], common::TimeUnit::Milliseconds);

    cal.total_trade_qty_trigger_time = 0.0;
    cal.total_nums_trigger_time = 0;
    cal.diff_price_nums_trigger_time = 0;
    cal.pre_100ms_nums = 0;
    cal.pre_100ms_diff_price_nums = 0;
    cal.pre_300ms_nums = 0;
    cal.pre_300ms_diff_price_nums = 0;
    cal.pre_500ms_nums = 0;
    cal.pre_500ms_diff_price_nums = 0;

    while (!trade_que.empty()) {
        auto cur_time = common::convertToTimestamp(trade_que.front()["data"]["T"], common::TimeUnit::Milliseconds);
        if (base_time - cur_time > 500) {
            trade_que.pop_front();
        } else {
            break;
        }
    }

    for (auto it = trade_que.rbegin(); it != trade_que.rend(); ++it) {
        const auto& cur = *it;
        auto        cur_time = common::convertToTimestamp(cur["data"]["T"], common::TimeUnit::Milliseconds);
        diff = base_time - cur_time;
        if (diff == 0) {
            if (pre_price != cur["data"]["p"] || cal.total_nums_trigger_time == 0) {
                ++cal.diff_price_nums_trigger_time;
            }
            ++cal.total_nums_trigger_time;
            cal.total_trade_qty_trigger_time += stod(cur["data"]["q"].get<string>());

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

std::string convertToUtc(const std::string& input) {
    std::tm tm = {};
    int     milliseconds;

    std::istringstream ss(input);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    ss.ignore(1, ',');
    ss >> milliseconds;

    // Convert to time_t and adjust for UTC+1 to UTC
    tm.tm_isdst = -1;                            // Not considering daylight saving time
    std::time_t time = std::mktime(&tm) - 3600;  // Subtract one hour

    // Convert back to tm structure
    std::tm* utc_tm = std::gmtime(&time);

    // Format the time string and replace the millisecond separator
    std::ostringstream result;
    result << std::put_time(utc_tm, "%Y-%m-%d %H:%M:%S");
    result << '.' << std::setw(3) << std::setfill('0') << milliseconds;

    return result.str();
}

void readCancellation(const vector<string>& files) {
    cout << "start readCancellation" << endl;
    // string base_trade_id = "4200000000";
    // string base_agg_id = "1700000000";
    for (const auto& file : files) {
        ifstream infile(file);
        if (!infile.is_open()) {
            cerr << "Error opening file: " << file << endl;
            continue;
        }
        cout << "cancle file :" << file << endl;

        string line;
        while (getline(infile, line)) {
            if (line.empty())
                continue;

            if (line.find("Trigger cancel, cancel id") != string::npos) {
                if (getSymbol(line) != log_symbol && !getSymbol(line).empty()) {
                    continue;
                }
                if (getType(line) != trigger_type && getType(line) != "deribit1s") {
                    continue;
                }
                if (line.find("Stream: trade") != string::npos && getSourceId(line) < base_trade_id) {
                    continue;
                }
                if (line.find("Stream: aggTrade") != string::npos && getSourceId(line) < base_agg_id) {
                    continue;
                }

                setCancelReq(line, file);
                ++total_cancel_no;
            } else if (line.find("Final Cancel Result") != string::npos) {
                setCancelInfo(line, file);
            }
        }
        cout << "trade_cancel size :" << trade_cancel.size() << ",agg cancel size:" << agg_cancel.size() << endl;
    }
    auto comp = [](const auto& a, const auto& b) { return a.id < b.id; };
    sort(trade_cancel.begin(), trade_cancel.end(), comp);
    sort(agg_cancel.begin(), agg_cancel.end(), comp);
}

void readTradeLog(const vector<string>& files, const string& prefix) {
    cout << "start readTradeLog" << endl;
    string resultPath = "./";
    string resultName = prefix + ".csv";
    string resuleFile = resultPath + resultName;

    ofstream tradeFile(resuleFile, ios::trunc);
    tradeFile << "calcel log time,"
              << "result,"
              << "trigger type,"
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
              << "processing time us,"
              << "deribit input time us,"
              << "deribit output time us";
    tradeFile << "\n";

    size_t idx = 0;

    for (const auto& file : files) {
        cout << "new file!!!!!!!!" << file << endl;
        ifstream infile(file);
        if (!infile.is_open()) {
            cerr << "Error opening file: " << file << endl;
            continue;
        }
        if (idx >= trade_cancel.size()) {
            break;
        }

        string pre_id("");
        string line;
        cout << "trade cancel size:" << trade_cancel.size() << ",idx:" << idx << ",id:" << trade_cancel[idx].id << endl;
        while (getline(infile, line) && idx < trade_cancel.size()) {
            if (line.empty())
                continue;

            line = line.substr(line.find("{"));
            json currentJson = json::parse(line, nullptr, false);
            trade_all.emplace_back(currentJson);
            while (trade_cancel[idx].id < pre_id) {
                ++idx;
            }
            if (trade_cancel[idx].id == to_string(currentJson["data"]["t"])) {
                cout << "idx:" << idx << ",sourceid:" << trade_cancel[idx].id << endl;

                CalculationInfo cal;
                getCalculationInfo(cal, trade_all);
                cout << " match trade cancel,id:[" << currentJson["data"]["t"] << "], trade id :" << trade_cancel[idx].id << endl;
                tradeFile << convertToUtc(trade_cancel[idx].logTime) << ",";
                tradeFile << trade_cancel[idx].result << ",";
                tradeFile << "trade,";
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
                tradeFile << common::timestampToDate(trade_cancel[idx].usIn, common::TimeUnit::Microseconds);
                tradeFile << common::timestampToDate(trade_cancel[idx].usOut, common::TimeUnit::Microseconds);
                tradeFile << "\n";

                pre_id = trade_cancel[idx].id;
                ++idx;
            }
        }
    }

    tradeFile.close();
}

void readAggTradeLog(const vector<string>& files, const string& prefix) {
    string resultPath = "./";
    string resultName = prefix + ".csv";
    string resuleFile = resultPath + resultName;

    ofstream tradeFile(resuleFile, ios::trunc);
    tradeFile << "calcel log time,"
              << "result,"
              << "trigger type,"
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
              << "processing time us,"
              << "deribit input time us,"
              << "deribit output time us";
    tradeFile << "\n";

    size_t idx = 0;

    for (const auto& file : files) {
        cout << "new file!!!!!!!!" << file << endl;
        ifstream infile(file);
        if (!infile.is_open()) {
            cerr << "Error opening file: " << file << endl;
            continue;
        }

        if (idx >= agg_cancel.size()) {
            break;
        }
        cout << "agg cancel size:" << agg_cancel.size() << ",idx:" << idx << ",id:" << agg_cancel[idx].id << endl;
        string pre_id("");
        string line;

        while (getline(infile, line) && idx < agg_cancel.size()) {
            if (line.empty())
                continue;

            line = line.substr(line.find("{"));
            json currentJson = json::parse(line, nullptr, false);
            aggtrade_all.emplace_back(currentJson);
            while (agg_cancel[idx].id < pre_id) {
                ++idx;
            }
            if (agg_cancel[idx].id == to_string(currentJson["data"]["a"])) {
                cout << "idx:" << idx << ",sourceid:" << agg_cancel[idx].id << endl;

                CalculationInfo cal;
                getCalculationInfo(cal, aggtrade_all);
                cout << " match trade cancel,id:[" << currentJson["data"]["a"] << "], trade id :" << agg_cancel[idx].id << endl;
                tradeFile << convertToUtc(agg_cancel[idx].logTime) << ",";
                tradeFile << agg_cancel[idx].result << ",";
                tradeFile << "aggtrade,";
                tradeFile << agg_cancel[idx].symbol << ",";    // symbol
                tradeFile << currentJson["data"]["q"] << ",";  // trigger qty
                tradeFile << currentJson["data"]["a"] << ",";  // trigger trade id
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

                tradeFile << agg_cancel[idx].type << ",";
                tradeFile << agg_cancel[idx].usDiff << ",";
                tradeFile << common::timestampToDate(agg_cancel[idx].usIn, common::TimeUnit::Microseconds);
                tradeFile << common::timestampToDate(agg_cancel[idx].usOut, common::TimeUnit::Microseconds);
                tradeFile << "\n";

                pre_id = agg_cancel[idx].id;
                ++idx;
            }
        }
    }

    tradeFile.close();
}

void readDeribit1sLog() {
    cout << "start readDeribit1sLog" << endl;

    ofstream tradeFile("./deribit1strade.csv", ios::trunc);
    tradeFile << "calcel log time,"
              << "result,"
              << "trigger type,"
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
              << "processing time us,"
              << "deribit input time us,"
              << "deribit output time us";
    tradeFile << "\n";

    size_t idx = 0;
    while (idx < within1s.size()) {
        tradeFile << convertToUtc(within1s[idx].logTime) << ",";
        tradeFile << within1s[idx].result << ",";
        tradeFile << "deribit within 1s trading,";
        tradeFile << within1s[idx].symbol << ",";  // symbol
        tradeFile << ",";                          // trigger qty
        tradeFile << ",";                          // trigger trade id
        tradeFile << ",";                          // trigger trade time at binance time
        // cal info
        tradeFile << ",";  // total trade nums at same binance time
        tradeFile << ",";  // total trade nums at same binance time
        tradeFile << ",";  // different price at same biance time
        tradeFile << ",";  // pre 100ms nums
        tradeFile << ",";  // pre 100ms diff price nums
        tradeFile << ",";
        tradeFile << ",";
        tradeFile << ",";
        tradeFile << ",";

        tradeFile << within1s[idx].type << ",";
        tradeFile << within1s[idx].usDiff << ",";
        tradeFile << common::timestampToDate(within1s[idx].usIn, common::TimeUnit::Microseconds);
        tradeFile << common::timestampToDate(within1s[idx].usOut, common::TimeUnit::Microseconds);
        tradeFile << "\n";

        ++idx;
    }

    tradeFile.close();
}

vector<string> getFilesWithPrefix(const string& dirPath, const string& prefix) {
    vector<string> files;
    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.is_regular_file() && entry.path().filename().string().find(prefix) == 0) {
            files.push_back(entry.path().string());
        }
    }

    auto comp = [](const std::string& a, const std::string& b) {
        int ia = 0;
        int ib = 0;

        size_t start_a = a.find('.') + 1;
        size_t end_a = a.find('.', start_a);
        if (end_a != std::string::npos) {
            ia = std::stoi(a.substr(start_a, end_a - start_a));
        }

        size_t start_b = b.find('.') + 1;
        size_t end_b = b.find('.', start_b);
        if (end_b != std::string::npos) {
            ib = std::stoi(b.substr(start_b, end_b - start_b));
        }

        return ia > ib;  // Use > for descending order
    };

    sort(files.begin(), files.end(), comp);

    return files;
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        cerr << "Usage: " << argv[0] << " <file_path> <cancelPrefix> <tradePrefix> <aggPrefix> <log_symbol> <trigger_type>" << endl;
        return 1;
    }

    string filePath = argv[1];
    string cancelPrefix = argv[2];
    string tradePrefix = argv[3];
    string aggPrefix = argv[4];

    log_symbol = argv[5];
    trigger_type = argv[6];

    cout << "filePath:" << filePath << ",cancelPrefix:" << cancelPrefix << ",tradePrefix:" << tradePrefix << ",aggPrefix:" << aggPrefix
         << ",log_symbol:" << log_symbol << ",trigger_type:" << trigger_type << endl;

    vector<string> cancelFiles = getFilesWithPrefix(filePath, cancelPrefix);
    vector<string> logFiles = getFilesWithPrefix(filePath, tradePrefix);
    vector<string> aggLogFiles = getFilesWithPrefix(filePath, aggPrefix);

    readCancellation(cancelFiles);

    readTradeLog(logFiles, tradePrefix);
    readAggTradeLog(aggLogFiles, aggPrefix);
    readDeribit1sLog();

    return 0;
}
