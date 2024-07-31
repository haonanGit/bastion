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
    int    start;
    int    end;
    string cancel_log_time;
};

vector<CancelInfo> cancel_all;
vector<string>     cancel_log;
string             title;
int                gap = 100;

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

    cancel_all.emplace_back(info);  // Using map with unique id as key
}

long long getLogTimestamp(const string& line) {
    std::stringstream ss(line);
    std::string       item;
    std::getline(ss, item, ',');  // cancel log time is the first item
    return stoll(item);
}

void getCalculationInfo(const long long& deribit_time, CalculationInfo& cal) {
    cal.start = -1;
    cal.end = -1;
    size_t idx = 0;
    while (idx < cancel_log.size()) {
        auto cur_time = getLogTimestamp(cancel_log[idx]);
        if (cal.start == -1 && (deribit_time - cur_time <= gap && deribit_time - cur_time >= 0)) {
            cal.start = idx;  // cancel before deribit trade
        } else if (cur_time - deribit_time <= gap && cur_time - deribit_time >= 0) {
            cal.end = idx;  // cancel after deribit trade
        } else if (cur_time - deribit_time > gap) {
            break;
        }
        ++idx;
    }
}

void readDeribit(const string& file) {
    cout << "deribit file :" << file << endl;
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

void readCancelLog(const string& file) {
    cout << "cancel file :" << file << endl;
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

        cancel_log.emplace_back(line);
    }
}

void mergeFile() {
    cout << "start merge" << endl;

    ofstream tradeFile("./cancel_trade.csv", ios::trunc);
    tradeFile << "deribit id,deribit trade time," << title;

    for (const auto& item : cancel_all) {
        CalculationInfo cal;
        getCalculationInfo(item.timestamp, cal);
        if (cal.start == -1) {
            tradeFile << item.id << ",";
            tradeFile << common::timestampToDate(item.timestamp, common::TimeUnit::Microseconds) << ",";
            cout << info.timestamp << "," << common::timestampToDate(item.timestamp, common::TimeUnit::Microseconds) << endl;
            tradeFile << "no cancel log match";
            tradeFile << "\n";
        }
        for (int i = cal.start; i < cal.end; ++i) {
            tradeFile << item.id << ",";
            tradeFile << common::timestampToDate(item.timestamp, common::TimeUnit::Microseconds) << ",";
            tradeFile << cancel_log[i];
        }
        tradeFile << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <cancel_file> <deribitfile>" << endl;
        return 1;
    }

    string cancel_file = argv[1];
    string deribitfile = argv[2];

    cout << "cancel_file:" << cancel_file << ",deribitfile:" << deribitfile << endl;

    readCancelLog(cancel_file);
    readDeribit(deribitfile);
    mergeFile();

    return 0;
}
