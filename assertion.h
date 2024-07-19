#ifndef ASSERTION_H
#define ASSERTION_H

#include <stdio.h>
#include <string_view>

// todo bool and int , pass by value or ref
inline void assert_log_impl(const bool& sentence, const std::string_view& msg, const char* file, const int& line) {
    if (!sentence) {
        fprintf(stderr, "%s: %d, ret[%.*s]\n", file, line, static_cast<int>(msg.size()), msg.data());
    }
}

#define ASSERT_LOG(sentence, msg) assert_log_impl((sentence), (msg), __FILE__, __LINE__)

#endif  // ASSERTION_H