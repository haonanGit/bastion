#ifndef RAW_KERNEL
#define RAW_KERNEL

#include "ws_client.h"

namespace app {

class RawKernelClient : public WSClient {
public:
    RawKernelClient() = default;
    ~RawKernelClient() = default;
    RawKernelClient(const std::string_view& addr, const std::string_view& subs_msg, const std::string_view& cancel_subs_msg)
        : WSClient(addr, subs_msg, cancel_subs_msg) {}
    void onMessage(websocketpp::connection_hdl hdl, app_client::message_ptr msg) override;
};
}  // namespace app

#endif  // RAW_KERNEL