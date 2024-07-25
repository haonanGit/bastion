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

    std::ofstream compareFile("/work/agg_trade/trade_aggtrade.txt", std::ios::trunc);

    std::string line;
    std::string agg_str;
    std::getline(infile, line);
    line = line.substr(line.find("{"));
    json currentJson = json::parse(line, nullptr, false);

    agg_str = m_agg.front();
    agg_str = agg_str.substr(agg_str.find("{"));
    json aggJson = json::parse(agg_str, nullptr, false);

    while (!line.empty() && !m_agg.empty()) {
        if (currentJson["data"]["t"] < aggJson["data"]["f"]) {
            std::getline(infile, line);
            if (line.empty())
                break;
            line = line.substr(line.find("{"));
            currentJson = json::parse(line, nullptr, false);
            continue;
        }

        if (currentJson["data"]["t"] > aggJson["data"]["l"]) {
            m_agg.pop();
            if (m_agg.empty())
                break;
            agg_str = m_agg.front();
            agg_str = agg_str.substr(agg_str.find("{"));
            aggJson = json::parse(agg_str, nullptr, false);
            continue;
        }

        // match first id
        bool                     write = false;
        std::vector<std::string> v;
        while (currentJson["data"]["t"] <= aggJson["data"]["l"]) {
            if (currentJson["received_t"] > aggJson["received_t"]) {
                write = true;
            }
            v.push_back(line);
            std::getline(infile, line);
            if (line.empty())
                break;
            line = line.substr(line.find("{"));
            currentJson = json::parse(line, nullptr, false);
        }

        if (write) {
            compareFile << agg_str << std::endl;
            for (auto& s : v) {
                compareFile << s << std::endl;
            }
            compareFile << std::endl << std::endl;
        }
        m_agg.pop();
        if (m_agg.empty())
            break;
        agg_str = m_agg.front();
        agg_str = agg_str.substr(agg_str.find("{"));
        aggJson = json::parse(agg_str, nullptr, false);
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
