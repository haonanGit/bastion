#include "common.h"
#include <assert.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdio.h>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>
#include "openssl/rand.h"

namespace common {

size_t calcDecodeLength(const char* b64input) {  // Calculates the length of a decoded string
    size_t len = strlen(b64input), padding = 0;

    if (b64input[len - 1] == '=' && b64input[len - 2] == '=')  // last two chars are =
        padding = 2;
    else if (b64input[len - 1] == '=')  // last char is =
        padding = 1;

    return (len * 3) / 4 - padding;
}

std::string toHexString(const unsigned char* hash, size_t length) {
    std::stringstream ss;
    for (size_t i = 0; i < length; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string sha256HexString(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX    sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.size());
    SHA256_Final(hash, &sha256);
    return toHexString(hash, SHA256_DIGEST_LENGTH);
}

std::string sha256RawString(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX    sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.size());
    SHA256_Final(hash, &sha256);
    return std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}

char* Base64Encode(const unsigned char* input, size_t length) {  // Encodes a binary safe base 64 string
    BIO *    bio, *b64;
    BUF_MEM* bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);  // Ignore newlines - write everything in one line
    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);

    char* buff = (char*)malloc(bufferPtr->length + 1);
    memcpy(buff, bufferPtr->data, bufferPtr->length);
    buff[bufferPtr->length] = 0;

    BIO_free_all(bio);

    return buff;
}

int Base64Decode(char* b64message, unsigned char** buffer, size_t* length) {  // Decodes a base64 encoded string
    BIO *bio, *b64;

    int decodeLen = calcDecodeLength(b64message);
    *buffer = (unsigned char*)malloc(decodeLen + 1);
    (*buffer)[decodeLen] = '\0';

    bio = BIO_new_mem_buf(b64message, -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);  // Do not use newlines to flush buffer
    *length = BIO_read(bio, *buffer, strlen(b64message));
    assert(*length == decodeLen);  // length should equal decodeLen, else something went horribly wrong
    BIO_free_all(bio);

    return (0);  // success
}

std::string GenerateRandomNonce(size_t length) {
    std::vector<unsigned char> buffer(length);
    RAND_bytes(buffer.data(), length);
    return Base64Encode(buffer.data(), buffer.size());
}

std::string timestampToDate(int64_t timestamp, TimeUnit unit) {
    int scale = 1000;
    int width = 3;
    switch (unit) {
    case TimeUnit::Milliseconds:
        scale = 1000;
        width = 3;
        break;
    case TimeUnit::Microseconds:
        scale = 1000000;
        width = 6;
        break;
    case TimeUnit::Nanoseconds:
        scale = 1000000000;
        width = 9;
        break;
    }
    std::time_t time = timestamp / scale;
    int         milliseconds = timestamp % scale;

    std::tm*          tm = std::gmtime(&time);
    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setw(width) << std::setfill('0') << milliseconds;

    return ss.str();
}

}  // namespace common