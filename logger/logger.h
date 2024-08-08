#ifndef LOGGER_H
#define LOGGER_H

#include "spdlog/async.h"
#include "spdlog/async_logger.h"
#include "spdlog/spdlog.h"
#include <memory>
#include <string>

namespace utils {

class Logger {
public:
  struct Options {
    std::string name{"default"};
    std::string file{"default.log"};
    std::string dir{"."};
    bool is_async{true};
    uint64_t max_size{1024L * 1024 * 100}; // 100 MB per file
    uint32_t max_files{100};               // up to 10 GB total
  };

  Logger() = default;
  explicit Logger(Options options);
  ~Logger();
  void init(const Options &options);
  void start();
  void start(const Options &opt);
  void info(const std::string_view &msg);
  void stop();
  void flush();

private:
  Options options_;
  std::shared_ptr<spdlog::logger> async_file_logger_;
  std::shared_ptr<spdlog::details::thread_pool> thread_pool_;
};

class GlobalLog {
public:
  static Logger &ins() {
    static std::shared_ptr<Logger> default_logger_ = std::make_shared<Logger>();
    return *default_logger_;
  }
};

} // namespace utils

#define LOG_INFO(msg)                                                          \
  do {                                                                         \
    utils::GlobalLog::ins().info(msg);                                         \
  } while (0)

#endif // LOGGER_H
