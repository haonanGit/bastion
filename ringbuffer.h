#include <array>
#include <atomic>

// sample ringbuffer

template <typename T, size_t buffer_size>
class RingBuffer {
public:
    // size_t buffer_size;
    char                padding0[56];
    std::atomic<size_t> read_idx;
    std::atomic<size_t> write_idx;
    // 0-empty 1-written 2-read
    std::array<std::atomic<size_t>, buffer_size> status;
    std::array<T, buffer_size>                   buffer;
    char                                         padding1[56];

public:
    RingBuffer() : read_idx(0), write_idx(0), status{}, buffer{} {
        std::fill(status.begin(), status.end(), 0);
    }
    ~RingBuffer() = default;

    bool write(T&& value) {
        auto current_write_idx = write_idx.load(std::memory_order_relaxed);
        auto next_write_idx = (current_write_idx + 1) % buffer_size;

        do {
            // if (1 == status[current_write_idx].load(std::memory_order_acquire)) {
            //   return false;
            // }
            next_write_idx = (current_write_idx + 1) % buffer_size;

        } while (!write_idx.compare_exchange_weak(current_write_idx, next_write_idx, std::memory_order_release, std::memory_order_relaxed));

        if (1 == status[current_write_idx].load(std::memory_order_acquire)) {
            return false;
        }

        // printf("write, current_write_idx idx:[%lu], read idx:[%lu]\n", current_write_idx, read_idx.load(std::memory_order_acquire));

        // memmove(buffer[current_write_idx].data, value, value->size);
        buffer[current_write_idx] = std::forward<T>(value);  // for both left and right value
        status[current_write_idx] = 1;
        return true;
    }

    bool read(T* value) {
        auto current_read_idx = read_idx.load(std::memory_order_relaxed);
        auto next_read_idx = (current_read_idx + 1) % buffer_size;

        do {
            // if (1 != status[current_read_idx].load(std::memory_order_acquire)) {
            //   return false;
            // }
            next_read_idx = (current_read_idx + 1) % buffer_size;

        } while (!read_idx.compare_exchange_weak(current_read_idx, next_read_idx, std::memory_order_release, std::memory_order_relaxed));

        if (1 != status[current_read_idx].load(std::memory_order_acquire)) {
            return false;
        }

        // printf("read, current_read_idx idx:[%lu], write idx:[%lu]\n", current_read_idx, write_idx.load(std::memory_order_acquire));

        // memcpy(value, buffer[current_read_idx].data, sizeof(T));
        // memcpy(value->payload, buffer[current_read_idx].data + sizeof(T), value->payload_size());
        *value = std::move(buffer[current_read_idx]);
        status[current_read_idx] = 2;
        return true;
    }
};