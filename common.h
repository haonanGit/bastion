#ifndef COMMON_H
#define COMMON_H

#include <chrono>
#include <string>

namespace common {

enum class TimeUnit { Milliseconds, Microseconds, Nanoseconds };

size_t calcDecodeLength(const char *b64input);
std::string toHexString(const unsigned char *hash, size_t length);
std::string sha256HexString(const std::string &data);
std::string sha256RawString(const std::string &data);
char *Base64Encode(const unsigned char *input, size_t length);
int Base64Decode(char *b64message, unsigned char **buffer, size_t *length);
std::string GenerateRandomNonce(size_t length);
std::string timestampToDate(int64_t timestamp,
                            TimeUnit unit = TimeUnit::Milliseconds);
long long convertToTimestamp(const std::string &dateTimeStr,
                             TimeUnit unit = TimeUnit::Milliseconds);
inline int64_t getTimeStampNs() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}
std::string hmac_sha256HexString(const std::string &secret,
                                 const std::string &sign);

} // namespace common

#endif // COMMON_H