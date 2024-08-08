#include <chrono>
#include <fstream>
#include "aggregate_trade_streams.h"
#include "common.h"
#include "nlohmann/json.hpp"
#include "vector"

int                    count = 0;
int                    threshold = 10000;
std::vector<long long> v_total(threshold);
std::vector<long long> v_send(threshold);
std::vector<long long> v_receive(threshold);

namespace app {

void AggregateTradeStreamsClient::onMessage(websocketpp::connection_hdl hdl, app_client::message_ptr msg) {
    auto        received_time = common::getNow();
    const auto& ret_msg = msg->get_payload();

    // Extract timestamp from the received message
    size_t separator_pos = payload.find('|');

    if (separator_pos != std::string::npos) {
        std::string client_time_str = payload.substr(0, separator_pos);
        std::string server_time_str = payload.substr(separator_pos + 1);

        int64_t client_time = std::stoll(client_time_str);
        int64_t server_time = std::stoll(server_time_str);

        int64_t client_to_server = server_time - client_time;
        int64_t server_to_client = received_time - server_time;

        // Store the round-trip time in the vector
        v_total[count] = v_time.push_back(client_to_server + server_to_client);

        std::cout << "Message received. Client to server: " << client_to_server << " ns, Server to client: " << server_to_client << " ns"
                  << std::endl;
    }

    ++count;
    if (count == threshold) {
        onCancelSubscription(hdl);
        stop();
    }
}

void wbSocketThread(const std::string_view& addr, const std::string_view& subs_msg, const std::string_view& cancel_subs_msg) {
    std::cout << "addr : " << addr << std::endl;
    AggregateTradeStreamsClient client(addr, subs_msg, cancel_subs_msg);
    client.start();
}

}  // namespace app

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string s_threhold = argv[1];
        threhold = std::stoi(s_threhold);
    }
    std::cout << "fetch records: " << threhold << std::endl;

    std::string addr = "wss://localhost:9000";  // Adjust to your local server address
    std::string subs_msg = R"({"method":"SUBSCRIBE","params":["local@trade"],"id":1})";
    std::string cancel_subs_msg = R"({"method":"UNSUBSCRIBE","params":["local@trade"],"id":1})";

    int                      thread_nums = 1;
    std::vector<std::thread> ws_threads;

    for (int i = 0; i < thread_nums; ++i) {
        ws_threads.emplace_back(app::wbSocketThread, addr, subs_msg, cancel_subs_msg);
    }

    for (auto& t : ws_threads) {
        t.join();
    }

    return 0;
}
