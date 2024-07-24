#include <benchmark/benchmark.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

// Global variables for loggers and thread pool
static std::shared_ptr<spdlog::logger>               sync_logger;
static std::shared_ptr<spdlog::logger>               async_logger;
static std::shared_ptr<spdlog::logger>               async_logger_ns;
static std::shared_ptr<spdlog::details::thread_pool> thread_pool;
static long long                                     total_duration = 0;

void setup_loggers() {
    // Setup synchronous logger
    auto sync_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("./log/sync_benchmark.log", 1024 * 1024 * 100, 10);
    sync_logger = std::make_shared<spdlog::logger>("sync_logger", sync_sink);
    sync_logger->set_pattern("%Y-%m-%dT%H:%M:%S.%fZ %l %v");

    // Setup asynchronous logger
    auto async_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("./log/async_benchmark.log", 1024 * 1024 * 100, 10);
    thread_pool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
    async_logger = std::make_shared<spdlog::async_logger>("async_logger", async_sink, thread_pool, spdlog::async_overflow_policy::overrun_oldest);
    async_logger->set_pattern("%Y-%m-%dT%H:%M:%S.%fZ %l %v");

    // Setup asynchronous logger with ns time measurement
    async_logger_ns =
        std::make_shared<spdlog::async_logger>("async_logger_ns", async_sink, thread_pool, spdlog::async_overflow_policy::overrun_oldest);
    async_logger_ns->set_pattern("%Y-%m-%dT%H:%M:%S.%fZ %l %v");
}

void benchmark_sync_logging(benchmark::State& state) {
    std::string log_message(state.range(0), 'A');
    spdlog::set_default_logger(sync_logger);
    for (auto _ : state) {
        spdlog::info(log_message);
    }
    sync_logger->flush();
}

void benchmark_async_logging(benchmark::State& state) {
    std::string log_message(state.range(0), 'A');
    spdlog::set_default_logger(async_logger);
    for (auto _ : state) {
        spdlog::info(log_message);
    }
    async_logger->flush();
}

void benchmark_async_logging_ns(benchmark::State& state) {
    std::string log_message(state.range(0), 'A');
    spdlog::set_default_logger(async_logger_ns);
    total_duration = 0;  // Reset total duration for each run

    for (auto _ : state) {
        auto start_time = std::chrono::high_resolution_clock::now();
        spdlog::info(log_message);
        auto end_time = std::chrono::high_resolution_clock::now();
        total_duration += std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

        benchmark::DoNotOptimize(total_duration);  // Prevent the compiler from optimizing the timing code away
    }
    async_logger_ns->flush();
    state.counters["AverageDuration(ns)"] = static_cast<double>(total_duration) / state.iterations();
}

static void BM_SyncLogging(benchmark::State& state) {
    benchmark_sync_logging(state);
}

static void BM_AsyncLogging(benchmark::State& state) {
    benchmark_async_logging(state);
}

static void BM_AsyncLogging_ns(benchmark::State& state) {
    benchmark_async_logging_ns(state);
}

BENCHMARK(BM_SyncLogging)->Arg(32)->Arg(128)->Arg(512)->Arg(1024)->Arg(4096)->Arg(16384);
BENCHMARK(BM_AsyncLogging)->Arg(32)->Arg(128)->Arg(512)->Arg(1024)->Arg(4096)->Arg(16384);
// BENCHMARK(BM_AsyncLogging_ns)->Arg(32)->Arg(128)->Arg(512)->Arg(1024)->Arg(4096)->Arg(16384);

int main(int argc, char** argv) {
    setup_loggers();  // Initialize loggers once
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    spdlog::shutdown();  // Ensure all loggers are properly shut down
    return 0;
}
