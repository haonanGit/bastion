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
    string    cur_time;
    long long timestamp;
    string    side;
    double    price;
    double    size;
    string    symbol;
    string    id;
};

struct CalculationInfo {
    int start;
    int end;
};

vector<CancelInfo>                    cancel_all;
unordered_map<string, vector<string>> total_log;
string                                title;
int                                   gap = 500;

std::string removeQuotes(const std::string& str) {
    std::string result = str;
    result.erase(std::remove(result.begin(), result.end(), '\"'), result.end());
    return result;
}

void setDeribitInfo(const std::string& cur) {
    CancelInfo        info;
    std::string       token;
    std::stringstream ss(cur);

    std::getline(ss, token, ',');
    info.cur_time = removeQuotes(token);

    std::getline(ss, token, ',');
    info.timestamp = std::stoll(removeQuotes(token));

    std::getline(ss, token, ',');
    info.side = removeQuotes(token);

    std::getline(ss, token, ',');
    info.price = std::stod(removeQuotes(token));

    std::getline(ss, token, ',');
    info.size = std::stod(removeQuotes(token));

    std::getline(ss, token, ',');
    info.symbol = removeQuotes(token);

    std::getline(ss, token, ',');
    info.id = removeQuotes(token);
    info.id.resize(info.id.size() - 1);  // remove '\n'

    cancel_all.emplace_back(info);  // Using map with unique id as key
}

long long getLogTimestamp(const string& line) {
    std::stringstream ss(line);
    std::string       item;
    std::getline(ss, item, ',');  // cancel log time is the first item
    return common::convertToTimestamp(item, common::TimeUnit::Milliseconds);
}

void getCalculationInfo(const long long& deribit_time, const vector<string>& log, CalculationInfo& cal) {
    cal.start = -1;
    cal.end = -1;
    size_t idx = 0;
    while (idx < log.size()) {
        auto cur_time = getLogTimestamp(log[idx]);

        if (cal.start == -1 && (deribit_time - cur_time <= gap && deribit_time - cur_time >= 0)) {
            cal.start = idx;  // cancel before deribit trade
            cal.end = idx;
        } else if (cur_time - deribit_time <= gap && cur_time - deribit_time >= 0) {
            cal.end = idx;  // cancel after deribit trade
        } else if (cur_time - deribit_time > gap) {
            break;
        }
        ++idx;
    }
}

void readDatabase(const string& file) {
    cout << "database file :" << file << endl;
    ifstream infile(file);
    if (!infile.is_open()) {
        cerr << "Error opening file: " << file << endl;
        return;
    }

    string line;
    while (getline(infile, line)) {
        if (line.empty() || line.find("cur_time") != string::npos) {
            continue;
        }

        setDeribitInfo(line);
    }

    auto comp = [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; };
    sort(cancel_all.begin(), cancel_all.end(), comp);
}

void readTradeLog(const string& file) {
    cout << "trade file :" << file << endl;
    ifstream infile(file);
    if (!infile.is_open()) {
        cerr << "Error opening file: " << file << endl;
        return;
    }

    string line;
    while (getline(infile, line)) {
        if (line.empty()) {
            continue;
        }
        if (line.find("result") != string::npos) {
            title = line;
            continue;
        }

        total_log["trade"].emplace_back(line);
    }
}

void readAggLog(const string& file) {
    cout << "agg file :" << file << endl;
    ifstream infile(file);
    if (!infile.is_open()) {
        cerr << "Error opening file: " << file << endl;
        return;
    }

    string line;
    while (getline(infile, line)) {
        if (line.empty()) {
            continue;
        }
        if (line.find("result") != string::npos) {
            title = line;
            continue;
        }

        total_log["agg"].emplace_back(line);
    }
}

void readDeribitLog(const string& file) {
    cout << "deribit file :" << file << endl;
    ifstream infile(file);
    if (!infile.is_open()) {
        cerr << "Error opening file: " << file << endl;
        return;
    }

    string line;
    while (getline(infile, line)) {
        if (line.empty()) {
            continue;
        }
        if (line.find("result") != string::npos) {
            title = line;
            continue;
        }

        total_log["deribit"].emplace_back(line);
    }
}

void mergeFile() {
    cout << "start merge" << endl;

    ofstream tradeFile("./cancel_trade.csv", ios::trunc);
    tradeFile << "deribit id,deribit trade time,deribit trade size," << title;

    // get all result
    for (const auto& item : cancel_all) {
        vector<string> result;

        unordered_map<string, CalculationInfo> cal;

        getCalculationInfo(item.timestamp, total_log["trade"], cal["trade"]);
        getCalculationInfo(item.timestamp, total_log["agg"], cal["agg"]);
        getCalculationInfo(item.timestamp, total_log["deribit"], cal["deribit"]);

        for (const auto& it : total_log) {
            stringstream ss;
            if (cal[it.first].start == -1) {
                continue;
            }

            for (int i = cal[it.first].start; i <= cal[it.first].end; ++i) {
                ss << item.id << ",";
                ss << common::timestampToDate(item.timestamp, common::TimeUnit::Milliseconds) << ",";
                ss << item.size << ",";
                // cout << "idx :" << i << ", size:" << trade_log.size() << endl;
                ss << it.second[i];
                ss << "\n\n";
            }
            if (!ss.str().empty()) {
                result.emplace_back(ss.str());
            }
        }

        for (auto s : result) {
            tradeFile << s;
        }
        if (result.empty()) {
            tradeFile << item.id << ",";
            tradeFile << common::timestampToDate(item.timestamp, common::TimeUnit::Milliseconds) << ",";
            tradeFile << item.size << ",";
            tradeFile << "no cancel log match\n";
            continue;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " <trade_file> <agg_file> <deribit1s_file> <database_file>" << endl;
        return 1;
    }

    string trade_file = argv[1];
    string agg_file = argv[2];
    string deribit1s_file = argv[3];
    string datebase_file = argv[4];

    cout << "trade_file:" << trade_file << ",agg_file:" << agg_file << ",deribit1s_file:" << deribit1s_file << ",datebase_file:" << datebase_file
         << endl;

    readTradeLog(trade_file);
    readAggLog(agg_file);
    readDeribitLog(deribit1s_file);
    readDatabase(datebase_file);
    mergeFile();

    return 0;
}
