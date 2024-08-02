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

#define trade_prefix "trade"
#define agg_prefix "agg"
#define deribit_prefix "deribit"

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

vector<CancelInfo>     cancel_all;
vector<vector<string>> total_log;
string                 title;
int                    gap = 500;

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

long long getDeribitCancelTime(const string& line) {
    std::stringstream ss(line);
    std::string       item;
    vector<string>    v;
    while (std::getline(ss, item, ',')) {
        v.emplace_back(item);
    }
    return common::convertToTimestamp(v[v.size() - 4], common::TimeUnit::Milliseconds);  // deribit cancel time is the last 4 item
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

void readTradeLog(const vector<string>& files) {
    for (const auto& file : files) {
        cout << "file :" << file << endl;
        vector<string> v;
        ifstream       infile(file);
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
            v.emplace_back(line);
        }
        total_log.emplace_back(v);
    }
}

void mergeFile() {
    cout << "start merge" << endl;

    ofstream tradeFile("./cancel_trade_result.csv", ios::trunc);
    tradeFile << "deribit id,"
              << "deribit trade time,"
              << "deribit trade size,"
              << "deribit cancel time - deribit trade time," << title;

    // get all result
    for (const auto& item : cancel_all) {
        vector<string> result;

        for (const auto& it : total_log) {
            CalculationInfo cal;
            getCalculationInfo(item.timestamp, it, cal);
            if (cal.start == -1) {
                continue;
            }

            stringstream ss;
            for (int i = cal.start; i <= cal.end; ++i) {
                ss << item.id << ",";
                ss << common::timestampToDate(item.timestamp, common::TimeUnit::Milliseconds) << ",";
                ss << item.size << ",";
                ss << getDeribitCancelTime(it[i]) - item.timestamp << ",";
                ss << it[i];
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

std::vector<std::string> getAllFilesInDirectory(const std::string& dirPath) {
    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <log_file_path:find all files> <database_path>" << endl;
        return 1;
    }

    string log_file_path = argv[1];
    string database_path = argv[2];

    cout << "log_file_path:" << log_file_path << ",database_path:" << database_path << endl;

    readTradeLog(getAllFilesInDirectory(log_file_path));

    readDatabase(database_path);
    mergeFile();

    return 0;
}
