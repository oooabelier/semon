#ifndef SEMON_PERSISTENCE_JSON_STREAMING_HPP
#define SEMON_PERSISTENCE_JSON_STREAMING_HPP

#include <cstddef>
#include <cstdint>

namespace semon {
namespace persistence {

class JsonWriter {
public:
    JsonWriter(bool (*write_fn)(const char* block, std::size_t length, void* userdata), void* userdata);
    JsonWriter(std::size_t (*write_func)(const char* data, std::size_t size, void* userdata), void* userdata);

    bool ok() const;
    bool writeRaw(const char* text);
    bool writeRaw(const char* text, std::size_t length);
    bool writeString(const char* text);
    bool writeNumber(std::uint64_t value);
    bool writeBool(bool value);
    bool writeKey(const char* key);

private:
    bool emit(const char* data, std::size_t size);
    bool escapeAndWrite(const char* text);

    bool (*write_fn_)(const char*, std::size_t, void*);
    std::size_t (*write_func_)(const char*, std::size_t, void*);
    void* userdata_;
    bool ok_;
};

class JsonReader {
public:
    explicit JsonReader(std::size_t (*read_fn)(char* buffer, std::size_t max_length, void* userdata), void* userdata);

    bool ok() const;
    bool eof() const;
    char peek();
    char get();
    bool consume(char expected);
    bool consume(const char* text);
    void skipWhitespace();
    bool readString(char* destination, std::size_t destination_size);
    bool readUnsigned(std::uint64_t* value);
    bool readBool(bool* value);

private:
    bool refill();
    bool setError(const char* message);

    std::size_t (*read_fn_)(char*, std::size_t, void*);
    void* userdata_;
    char buffer_[256];
    std::size_t position_;
    std::size_t length_;
    bool eof_;
    bool ok_;
    char error_[64];
};

} // namespace persistence
} // namespace semon

#endif
