#include "persistence/JsonStreaming.hpp"

#include <cstdio>
#include <cstring>

namespace semon {
namespace persistence {

JsonWriter::JsonWriter(bool (*write_fn)(const char* block, std::size_t length, void* userdata), void* userdata)
    : write_fn_(write_fn),
      write_func_(static_cast<std::size_t (*)(const char*, std::size_t, void*)>(0)),
      userdata_(userdata),
      ok_(write_fn != static_cast<bool (*)(const char*, std::size_t, void*)>(0))
{
}

JsonWriter::JsonWriter(std::size_t (*write_func)(const char* data, std::size_t size, void* userdata), void* userdata)
    : write_fn_(static_cast<bool (*)(const char*, std::size_t, void*)>(0)),
      write_func_(write_func),
      userdata_(userdata),
      ok_(write_func != static_cast<std::size_t (*)(const char*, std::size_t, void*)>(0))
{
}

bool JsonWriter::ok() const
{
    return ok_;
}

bool JsonWriter::writeRaw(const char* text)
{
    return text != static_cast<const char*>(0) && writeRaw(text, std::strlen(text));
}

bool JsonWriter::writeRaw(const char* text, std::size_t length)
{
    return emit(text, length);
}

bool JsonWriter::writeString(const char* text)
{
    if (!emit("\"", 1U)) {
        return false;
    }
    if (!escapeAndWrite(text)) {
        return false;
    }
    return emit("\"", 1U);
}

bool JsonWriter::writeNumber(std::uint64_t value)
{
    char buffer[32];
    const int written = std::snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
    if (written <= 0) {
        ok_ = false;
        return false;
    }
    return emit(buffer, static_cast<std::size_t>(written));
}

bool JsonWriter::writeBool(bool value)
{
    return value ? emit("true", 4U) : emit("false", 5U);
}

bool JsonWriter::writeKey(const char* key)
{
    if (!writeString(key)) {
        return false;
    }
    return emit(":", 1U);
}

bool JsonWriter::emit(const char* data, std::size_t size)
{
    if (!ok_ || data == static_cast<const char*>(0)) {
        ok_ = false;
        return false;
    }

    if (write_fn_ != static_cast<bool (*)(const char*, std::size_t, void*)>(0)) {
        ok_ = write_fn_(data, size, userdata_);
    } else {
        const std::size_t written = write_func_(data, size, userdata_);
        ok_ = written == size;
    }
    return ok_;
}

bool JsonWriter::escapeAndWrite(const char* text)
{
    if (text == static_cast<const char*>(0)) {
        return true;
    }

    const char* current = text;
    while (*current != '\0') {
        if (*current == '"' || *current == '\\') {
            if (!emit("\\", 1U) || !emit(current, 1U)) {
                return false;
            }
        } else if (*current == '\n') {
            if (!emit("\\n", 2U)) {
                return false;
            }
        } else if (*current == '\r') {
            if (!emit("\\r", 2U)) {
                return false;
            }
        } else if (*current == '\t') {
            if (!emit("\\t", 2U)) {
                return false;
            }
        } else {
            if (!emit(current, 1U)) {
                return false;
            }
        }
        current += 1;
    }
    return true;
}

JsonReader::JsonReader(std::size_t (*read_fn)(char* buffer, std::size_t max_length, void* userdata), void* userdata)
    : read_fn_(read_fn),
      userdata_(userdata),
      buffer_(),
      position_(0U),
      length_(0U),
      eof_(false),
      ok_(read_fn != static_cast<std::size_t (*)(char*, std::size_t, void*)>(0)),
      error_()
{
}

bool JsonReader::ok() const
{
    return ok_;
}

bool JsonReader::eof() const
{
    return eof_;
}

char JsonReader::peek()
{
    if (!refill()) {
        return '\0';
    }
    return buffer_[position_];
}

char JsonReader::get()
{
    if (!refill()) {
        return '\0';
    }
    const char value = buffer_[position_];
    position_ += 1U;
    return value;
}

bool JsonReader::consume(char expected)
{
    skipWhitespace();
    if (peek() != expected) {
        return false;
    }
    (void)get();
    return true;
}

bool JsonReader::consume(const char* text)
{
    skipWhitespace();
    const std::size_t length = std::strlen(text);
    for (std::size_t index = 0U; index < length; ++index) {
        if (get() != text[index]) {
            return setError("expected token");
        }
    }
    return true;
}

void JsonReader::skipWhitespace()
{
    while (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r') {
        (void)get();
    }
}

bool JsonReader::readString(char* destination, std::size_t destination_size)
{
    if (destination == static_cast<char*>(0) || destination_size == 0U) {
        return false;
    }
    skipWhitespace();
    if (!consume('"')) {
        return setError("expected string");
    }

    std::size_t index = 0U;
    while (!eof_) {
        char current = get();
        if (current == '"') {
            destination[index] = '\0';
            return true;
        }
        if (current == '\\') {
            const char escaped = get();
            if (escaped == 'n') {
                current = '\n';
            } else if (escaped == 'r') {
                current = '\r';
            } else if (escaped == 't') {
                current = '\t';
            }
        }
        if ((index + 1U) < destination_size) {
            destination[index] = current;
            index += 1U;
        }
    }
    return setError("unterminated string");
}

bool JsonReader::readUnsigned(std::uint64_t* value)
{
    if (value == static_cast<std::uint64_t*>(0)) {
        return false;
    }
    skipWhitespace();
    std::uint64_t result = 0ULL;
    bool found = false;
    while (peek() >= '0' && peek() <= '9') {
        result = (result * 10ULL) + static_cast<std::uint64_t>(get() - '0');
        found = true;
    }
    if (!found) {
        return setError("expected number");
    }
    *value = result;
    return true;
}

bool JsonReader::readBool(bool* value)
{
    if (value == static_cast<bool*>(0)) {
        return false;
    }
    if (consume("true")) {
        *value = true;
        return true;
    }
    if (consume("false")) {
        *value = false;
        return true;
    }
    return setError("expected bool");
}

bool JsonReader::refill()
{
    if (!ok_) {
        return false;
    }
    if (position_ < length_) {
        return true;
    }
    if (eof_) {
        return false;
    }

    length_ = read_fn_(buffer_, sizeof(buffer_), userdata_);
    position_ = 0U;
    if (length_ == 0U) {
        eof_ = true;
        return false;
    }
    return true;
}

bool JsonReader::setError(const char* message)
{
    ok_ = false;
    if (message != static_cast<const char*>(0)) {
        std::size_t index = 0U;
        while ((index + 1U) < sizeof(error_) && message[index] != '\0') {
            error_[index] = message[index];
            index += 1U;
        }
        error_[index] = '\0';
    } else {
        error_[0] = '\0';
    }
    return false;
}

} // namespace persistence
} // namespace semon
