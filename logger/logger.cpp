#include "logger.h"
#include <iostream>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace utils {

Logger::Logger(Options options) : options_(std::move(options)) {
  init(options_);
}

Logger::~Logger() { stop(); }

void Logger::init(const Options &options) {
  options_ = options;
  std::string file_path = options.dir + "/" + options.file;

  auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      file_path, options.max_size, options.max_files, true);
  if (options.is_async) {
    thread_pool_ = std::make_shared<spdlog::details::thread_pool>(1024 * 5, 1);
    async_file_logger_ = std::make_shared<spdlog::async_logger>(
        options.name, sink, thread_pool_,
        spdlog::async_overflow_policy::overrun_oldest);
    LOG_INFO(options.name + "is async log mod");
  } else {
    async_file_logger_ = std::make_shared<spdlog::logger>(options.name, sink);
  }
  spdlog::set_default_logger(async_file_logger_);
  async_file_logger_->set_pattern("%Y-%m-%dT%H:%M:%S.%fZ %l %t %v");

  start(options);
}

void Logger::start() {
  if (options_.is_async) {
    // async_file_logger_->flush_on(spdlog::level::info);
    // LOG_INFO(options_.name + "is async log mod\n");
  }
}

void Logger::start(const Options &opt) {
  if (opt.is_async) {
    // async_file_logger_->flush_on(spdlog::level::info);
    LOG_INFO(opt.name + "is async log mod\n");
    async_file_logger_->flush();
  }
}

void Logger::info(const std::string_view &msg) {
  async_file_logger_->info(msg);
}

void Logger::stop() {
  if (async_file_logger_) {
    async_file_logger_->flush();
    async_file_logger_.reset();
    spdlog::drop(options_.name);
  }
  if (thread_pool_) {
    thread_pool_.reset();
  }
}

void Logger::flush() {
  if (async_file_logger_) {
    async_file_logger_->flush();
  }
}
} // namespace utils
