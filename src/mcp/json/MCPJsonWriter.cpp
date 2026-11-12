#include "MCPJsonWriter.h"
#include <string.h>
#include <stdio.h>

static const char HEX_DIGITS[] = "0123456789abcdef";

MCPJsonWriter::MCPJsonWriter(char* buffer, size_t capacity)
    : _buffer(buffer), _capacity(capacity), _size(0), _overflow(false), _depth(0) {
    reset();
}

void MCPJsonWriter::reset() {
    _size = 0;
    _overflow = false;
    _depth = 0;
    for (int i = 0; i < MCP_MAX_JSON_DEPTH; ++i) {
        _needsComma[i] = false;
    }
    if (_buffer && _capacity > 0) {
        _buffer[0] = '\0';
    }
}

const char* MCPJsonWriter::c_str() const {
    if (!_buffer || _capacity == 0) return "";
    return _buffer;
}

bool MCPJsonWriter::appendChar(char c) {
    if (_overflow || !_buffer) return false;
    if (_size + 1 >= _capacity) {
        _overflow = true;
        _buffer[_capacity - 1] = '\0';
        return false;
    }
    _buffer[_size++] = c;
    _buffer[_size] = '\0';
    return true;
}

bool MCPJsonWriter::appendBytes(const char* bytes, size_t count) {
    if (_overflow || !_buffer) return false;
    if (!bytes || count == 0) return true;
    if (_size + count >= _capacity) {
        _overflow = true;
        _buffer[_capacity - 1] = '\0';
        return false;
    }
    memcpy(_buffer + _size, bytes, count);
    _size += count;
    _buffer[_size] = '\0';
    return true;
}

void MCPJsonWriter::writeCommaIfNeeded() {
    if (_depth > 0 && _depth < MCP_MAX_JSON_DEPTH) {
        if (_needsComma[_depth]) {
            appendChar(',');
        } else {
            _needsComma[_depth] = true;
        }
    }
}

bool MCPJsonWriter::startObject() {
    writeCommaIfNeeded();
    if (!appendChar('{')) return false;
    if (_depth + 1 < MCP_MAX_JSON_DEPTH) {
        _depth++;
        _needsComma[_depth] = false;
    }
    return !_overflow;
}

bool MCPJsonWriter::endObject() {
    if (_depth > 0) {
        _depth--;
    }
    return appendChar('}');
}

bool MCPJsonWriter::startArray() {
    writeCommaIfNeeded();
    if (!appendChar('[')) return false;
    if (_depth + 1 < MCP_MAX_JSON_DEPTH) {
        _depth++;
        _needsComma[_depth] = false;
    }
    return !_overflow;
}

bool MCPJsonWriter::endArray() {
    if (_depth > 0) {
        _depth--;
    }
    return appendChar(']');
}

bool MCPJsonWriter::writeKey(const char* key) {
    if (!key) return false;
    writeCommaIfNeeded();
    if (!writeStringEscaped(key, strlen(key))) return false;
    return appendChar(':');
}

bool MCPJsonWriter::writeStringEscaped(const char* str, size_t len) {
    if (!str) return writeNull();
    if (len == 0) len = strlen(str);

    if (!appendChar('"')) return false;
    for (size_t i = 0; i < len; ++i) {
        char c = str[i];
        if (c == '"') {
            if (!appendBytes("\\\"", 2)) return false;
        } else if (c == '\\') {
            if (!appendBytes("\\\\", 2)) return false;
        } else if (c == '/') {
            if (!appendBytes("\\/", 2)) return false;
        } else if (c == '\b') {
            if (!appendBytes("\\b", 2)) return false;
        } else if (c == '\f') {
            if (!appendBytes("\\f", 2)) return false;
        } else if (c == '\n') {
            if (!appendBytes("\\n", 2)) return false;
        } else if (c == '\r') {
            if (!appendBytes("\\r", 2)) return false;
        } else if (c == '\t') {
            if (!appendBytes("\\t", 2)) return false;
        } else if ((unsigned char)c < 0x20) {
            char ubuf[7];
            ubuf[0] = '\\';
            ubuf[1] = 'u';
            ubuf[2] = '0';
            ubuf[3] = '0';
            ubuf[4] = HEX_DIGITS[((unsigned char)c >> 4) & 0x0F];
            ubuf[5] = HEX_DIGITS[(unsigned char)c & 0x0F];
            ubuf[6] = '\0';
            if (!appendBytes(ubuf, 6)) return false;
        } else {
            if (!appendChar(c)) return false;
        }
    }
    return appendChar('"');
}

bool MCPJsonWriter::writeString(const char* val) {
    writeCommaIfNeeded();
    if (!val) return writeNull();
    return writeStringEscaped(val, strlen(val));
}

bool MCPJsonWriter::writeRaw(const char* raw, size_t len) {
    writeCommaIfNeeded();
    if (!raw || len == 0) return writeNull();
    return appendBytes(raw, len);
}

bool MCPJsonWriter::writeLong(long val) {
    writeCommaIfNeeded();
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "%ld", val);
    if (n <= 0) return false;
    return appendBytes(buf, (size_t)n);
}

bool MCPJsonWriter::writeDouble(double val, int decimals) {
    writeCommaIfNeeded();
    char buf[48];
    int n = snprintf(buf, sizeof(buf), "%.*f", decimals, val);
    if (n <= 0) return false;
    return appendBytes(buf, (size_t)n);
}

bool MCPJsonWriter::writeBool(bool val) {
    writeCommaIfNeeded();
    return val ? appendBytes("true", 4) : appendBytes("false", 5);
}

bool MCPJsonWriter::writeNull() {
    writeCommaIfNeeded();
    return appendBytes("null", 4);
}

bool MCPJsonWriter::addString(const char* key, const char* val) {
    if (!writeKey(key)) return false;
    return writeStringEscaped(val, val ? strlen(val) : 0);
}

bool MCPJsonWriter::addRaw(const char* key, const char* rawJson, size_t rawLen) {
    if (!writeKey(key)) return false;
    return appendBytes(rawJson, rawLen);
}

bool MCPJsonWriter::addLong(const char* key, long val) {
    if (!writeKey(key)) return false;
    return writeLong(val);
}

bool MCPJsonWriter::addDouble(const char* key, double val, int decimals) {
    if (!writeKey(key)) return false;
    return writeDouble(val, decimals);
}

bool MCPJsonWriter::addBool(const char* key, bool val) {
    if (!writeKey(key)) return false;
    return writeBool(val);
}

bool MCPJsonWriter::addNull(const char* key) {
    if (!writeKey(key)) return false;
    return writeNull();
}
