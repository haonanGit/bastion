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

std::queue<std::string> m_agg;

void readAggTrade(const std::string& file) {
    std::ifstream infile(file);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << file << std::endl;
        return;
    }

    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty())
            continue;

        json currentJson = json::parse(line, nullptr, false);
        m_agg.push(line);
        // m_agg[currentJson["qty"]] = line;
    }
}

void processFile(const std::string& tradefile) {
    std::ifstream infile(tradefile);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << tradefile << std::endl;
        return;
    }

    std::ofstream compareFile("trade_aggtrade.json", std::ios::trunc);

    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty())
            continue;

        json currentJson = json::parse(line, nullptr, false);

        std::string sum_qty = currentJson["sum_qty"];
        if (!m_agg.empty()) {
            auto aggLine = m_agg.front();
            json aggJson = json::parse(aggLine, nullptr, false);
            auto trade_e_time = currentJson["e_time"];
            auto agg_e_time = aggJson["e_time"];
            auto trade_sum_qty = currentJson["sum_qty"];
            auto agg_qty = aggJson["qty"];
            int  trade_count = currentJson["count"];
            int  agg_count = aggJson["count"];
            if (trade_e_time > agg_e_time) {
                std::cout << "time" << std::endl;
                compareFile << line << " : time_trade" << std::endl;
                compareFile << m_agg.front() << " : time_aggtrade" << std::endl << std::endl;
                m_agg.pop();
            } else if (trade_count > agg_count) {
                std::cout << "count" << std::endl;
                compareFile << line << " : count_trade" << std::endl;
                compareFile << m_agg.front() << " : count_aggtrade" << std::endl << std::endl;
                m_agg.pop();
            } else if (trade_sum_qty == agg_qty) {
                m_agg.pop();
            }
        }
    }

    infile.close();
    compareFile.close();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <trade_file_path> <aggtrade_file_path>" << std::endl;
        return 1;
    }

    std::string tradefile = argv[1];
    std::string aggtradefile = argv[2];

    readAggTrade(aggtradefile);
    processFile(tradefile);

    return 0;
}
