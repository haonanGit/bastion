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

using json = nlohmann::json;

std::queue<std::string> aggQty;

bool areDoublesEqual(double a, double b, double epsilon = 1e-9) {
    return std::fabs(a - b) < epsilon;
}

std::string formatTimestamp(long long timestamp) {
    std::time_t       t = timestamp / 1000;
    long long         milliseconds = timestamp % 1000;
    std::tm           tm = *std::gmtime(&t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%H:%M:%S") << "." << std::setw(3) << std::setfill('0') << milliseconds;
    return ss.str();
}

void convertTimestampToDate(nlohmann::json& jsonObject) {
    // Aggregate Trade Streams
    if (jsonObject.contains("data") && !jsonObject["data"].is_null() && jsonObject["data"].contains("E") && !jsonObject["data"]["E"].is_null() &&
        jsonObject["data"].contains("T") && !jsonObject["data"]["T"].is_null()) {
        long long   E = jsonObject["data"]["E"];
        long long   T = jsonObject["data"]["T"];
        std::string E_date = common::timestampToDate(E).substr(11);
        std::string T_date = common::timestampToDate(T).substr(11);
        jsonObject["data"]["E"] = E_date;
        jsonObject["data"]["T"] = T_date;
    }
}

bool isWithinTimeRange(const std::string& tradeTimeStr, const std::string& startTime, const std::string& endTime) {
    return tradeTimeStr >= startTime && tradeTimeStr <= endTime;
}

void processFile(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::ofstream tradeFile("trade.json", std::ios::trunc);
    std::ofstream aggtradeFile("aggtrade.json", std::ios::trunc);
    std::ofstream tickerFile("ticker.json", std::ios::trunc);
    // std::ofstream depthFile("depth.json", std::ios::trunc);

    std::string line;
    std::string previousTickerTime;
    std::string previousTradePrice;
    std::string previousAggTradePrice;
    std::string startTime("00:00:00.000");
    std::string endTime("23:59:59.999");
    double      tradeSum = 0.0;
    // aggtrade
    while (std::getline(infile, line)) {
        if (line.empty())
            continue;

        json currentJson = json::parse(line, nullptr, false);
        if (currentJson.is_discarded() || !currentJson.contains("data") || !currentJson["data"].contains("T")) {
            continue;
        }

        // long long   tradeTime = currentJson["data"]["T"];
        // long long   eventTime = currentJson["data"]["E"];
        std::string tradeTimeStr = currentJson["data"]["T"];  // 2024-07-05 04:34:13.487

        if (isWithinTimeRange(tradeTimeStr.substr(11), startTime, endTime)) {
            std::string streamType = currentJson["stream"];

            // convertTimestampToDate(currentJson);

            if (streamType == "xrpusdt@aggTrade") {
                // aggtradeFile << currentJson.dump(4) << std::endl;
                std::string t_time = currentJson["data"]["T"];
                std::string e_time = currentJson["data"]["E"];
                std::string price = currentJson["data"]["p"];
                std::string qty = currentJson["data"]["q"];
                auto        count = currentJson["count"];

                aggQty.push(qty);
                aggtradeFile << "{ \"t_time\": \"" << t_time << "\", \"e_time\": \"" << e_time << "\", \"price\": \"" << price << "\", \"qty\": \""
                             << qty << "\" , \"count\": " << count << " }" << std::endl;
                aggtradeFile << std::endl;
            } else if (streamType == "xrpusdt@bookTicker") {
                // // tickerFile << currentJson.dump(4) << std::endl;
                // std::string t_time = currentJson["data"]["T"];
                // std::string e_time = currentJson["data"]["E"];
                // std::string bidPrice = currentJson["data"]["b"];
                // std::string bidQty = currentJson["data"]["B"];
                // std::string askPrice = currentJson["data"]["a"];
                // std::string askQty = currentJson["data"]["A"];

                // if (!previousTickerTime.empty() && previousTickerTime != t_time) {
                //     tickerFile << std::endl;
                // }

                // tickerFile << "{ \"t_time\": \"" << t_time << "\", \"e_time\": \"" << e_time << "\", \"bid-price\": \"" << bidPrice << "\",
                // \"bid-qty\": \"" << bidQty
                //            << "\", \"ask-price\": \"" << askPrice << "\", \"ask-qty\": \"" << askQty << "\" }" << std::endl;
                // previousTickerTime = t_time;
            } else if (streamType == "xrpusdt@depth5") {
                // depthFile << currentJson.dump(4) << std::endl;
            }
        }
    }

    // Reopen the file
    if (infile.is_open()) {
        infile.close();
    }
    infile.open(filename);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }
    std::cout << "start trade\n";
    // trade
    previousTradePrice = "";
    while (std::getline(infile, line)) {
        if (line.empty())
            continue;

        json currentJson = json::parse(line, nullptr, false);
        if (currentJson.is_discarded() || !currentJson.contains("data") || !currentJson["data"].contains("T")) {
            continue;
        }

        // long long   tradeTime = currentJson["data"]["T"];
        // long long   eventTime = currentJson["data"]["E"];
        std::string tradeTimeStr = currentJson["data"]["T"];  // 2024-07-05 04:34:13.487

        if (isWithinTimeRange(tradeTimeStr.substr(11), startTime, endTime)) {
            std::string streamType = currentJson["stream"];
            // convertTimestampToDate(currentJson);

            if (streamType == "xrpusdt@trade") {
                // Only market trades will be aggregated, which means the insurance fund trades and ADL trades won't be aggregated.
                if (currentJson["data"]["X"] != "MARKET")
                    continue;

                std::string t_time = currentJson["data"]["T"];
                std::string e_time = currentJson["data"]["E"];
                std::string price = currentJson["data"]["p"];
                std::string qty = currentJson["data"]["q"];
                auto        count = currentJson["count"];
                // if (!previousTradePrice.empty() && previousTradePrice != price) {
                //     tradeFile << std::endl;
                //     tradeSum = 0.0;
                // }

                previousTradePrice = price;
                tradeSum += std::stod(qty);
                tradeFile << "{ \"t_time\": \"" << t_time << "\", \"e_time\": \"" << e_time << "\", \"price\": \"" << price << "\", \"qty\": \""
                          << qty << "\", \"sum_qty\": \"" << std::fixed << std::setprecision(1) << tradeSum << "\", \"count\": " << count << " }"
                          << std::endl;

                if (!aggQty.empty() && areDoublesEqual(tradeSum, std::stod(aggQty.front()))) {
                    aggQty.pop();
                    tradeSum = 0.0;
                    tradeFile << std::endl;
                }
            }
        }
    }

    infile.close();
    tradeFile.close();
    aggtradeFile.close();
    // depthFile.close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file_path>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    processFile(filename);

    return 0;
}
