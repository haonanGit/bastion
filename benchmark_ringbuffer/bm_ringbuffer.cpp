#include <benchmark/benchmark.h>
#include <iostream>
#include <thread>
#include "ringbuffer.h"

constexpr size_t BUFFER_SIZE = 16;

class TestData {
private:
    std::string _url;

public:
    TestData() = default;
    TestData(const std::string& url) : _url(url) {}

    TestData& operator=(const TestData& data) {
        if (this != &data) {
            _url = data._url;
        }
        return *this;
    }

    TestData& operator=(TestData&& data) noexcept {
        if (this != &data) {
            _url = std::move(data._url);
        }
        return *this;
    }
};

void Writer(RingBuffer<TestData, BUFFER_SIZE>& buffer, benchmark::State& state) {
    for (auto _ : state) {
        TestData data("test url = www.baidu.com");
        while (!buffer.write(std::move(data))) {
            // spin wait;
        }
    }
}

void Reader(RingBuffer<TestData, BUFFER_SIZE>& buffer, benchmark::State& state) {
    for (auto _ : state) {
        TestData data("");
        while (!buffer.read(&data)) {
            // spin wait;
        }
    }
}

static void BM_WriteRead(benchmark::State& state) {
    RingBuffer<TestData, BUFFER_SIZE> buffer;

    std::thread th_writer1(Writer, std::ref(buffer), std::ref(state));
    std::thread th_reader1(Reader, std::ref(buffer), std::ref(state));
    // std::thread th_writer2(Writer, std::ref(buffer), std::ref(state));
    // std::thread th_reader2(Reader, std::ref(buffer), std::ref(state));

    th_writer1.join();
    th_reader1.join();
    // th_writer2.join();
    // th_reader2.join();
}

// Register the function as a benchmark
BENCHMARK(BM_WriteRead);
// Run the benchmark
BENCHMARK_MAIN();