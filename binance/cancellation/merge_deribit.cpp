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

vector<CancelInfo> cancel_all;

void setDeribitInfo(const string& cur) {
    CancelInfo  info;
    std::string token;

    std::getline(ss, token, ',');
    info.cur_time = token;

    std::getline(ss, token, ',');
    info.timestamp = std::stoll(token);

    std::getline(ss, token, ',');
    info.side = token;

    std::getline(ss, token, ',');
    info.price = std::stod(token);

    std::getline(ss, token, ',');
    info.size = std::stod(token);

    std::getline(ss, token, ',');
    info.symbol = token;

    std::getline(ss, token, ',');
    info.id = token;

    cancel_all.emplace_back(info);
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

void readDeribit(const vector<string>& files) {
    cout << "start readDeribit" << endl;
    for (const auto& file : files) {
        ifstream infile(file);
        if (!infile.is_open()) {
            cerr << "Error opening file: " << file << endl;
            continue;
        }
        cout << "deribit file :" << file << endl;

        string line;
        while (getline(infile, line)) {
            if (line.empty() || line.find("cur_time") != sting::string::npos) {
                continue;
            }

            setDeribitInfo(line);
        }
        cout << "cancel_all size :" << cancel_all.size() << endl;
    }
    auto comp = [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; };
    sort(trade_cancel.begin(), trade_cancel.end(), comp);
}

void readTradeLog(const vector<string>& files) {
    cout << "start readTradeLog" << endl;

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
        string pre_id("");
        string line;
        cout << "trade cancel size:" << trade_cancel.size() << ",idx:" << idx << endl;
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
                getCalculationInfo(cal);
                cout << " match trade cancel,id:[" << currentJson["data"]["t"] << "]" << endl;
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
                tradeFile << common::timestampToDate(trade_cancel[idx].usIn, common::TimeUnit::Microseconds);
                tradeFile << common::timestampToDate(trade_cancel[idx].usOut, common::TimeUnit::Microseconds);
                tradeFile << "\n";

                pre_id = trade_cancel[idx].id;
                ++idx;
            }
        }
        if (idx < trade_cancel.size())
            cout << "cur idx:" << trade_cancel[idx].id << endl;
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
        cerr << "Usage: " << argv[0] << " <file_path> <cancel_file_prefix> <log_file_prefix> <log_type> <symbol> <trigger_type>" << endl;
        return 1;
    }

    string filePath = argv[1];
    string cancelPrefix = argv[2];
    string logPrefix = argv[3];
    string log_type = argv[4];

    log_symbol = argv[5];
    trigger_type = argv[6];

    cout << "filePath:" << filePath << ",cancelPrefix:" << cancelPrefix << ",logPrefix:" << logPrefix << ",log_type" << log_type
         << ",log_symbol:" << log_symbol << "trigger_type" << trigger_type << endl;

    vector<string> cancelFiles = getFilesWithPrefix(filePath, cancelPrefix);
    vector<string> logFiles = getFilesWithPrefix(filePath, logPrefix);

    readCancellation(cancelFiles);
    if (log_type == "trade")
        readTradeLog(logFiles);

    return 0;
}
