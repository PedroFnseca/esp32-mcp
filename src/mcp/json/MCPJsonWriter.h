#ifndef ESP32_MCP_JSON_WRITER_H
#define ESP32_MCP_JSON_WRITER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "MCPJsonConfig.h"

class MCPJsonWriter {
private:
    enum ContainerType {
        CONTAINER_NONE = 0,
        CONTAINER_OBJECT,
        CONTAINER_ARRAY
    };

    char* _buffer;
    size_t _capacity;
    size_t _size;
    bool _overflow;
    ContainerType _containerStack[MCP_MAX_JSON_DEPTH];
    bool _needsComma[MCP_MAX_JSON_DEPTH];
    bool _inKey[MCP_MAX_JSON_DEPTH];
    int _depth;

    bool appendChar(char c);
    bool appendBytes(const char* bytes, size_t count);
    void writeCommaIfNeeded();
    void prepareValue();

public:
    MCPJsonWriter(char* buffer, size_t capacity);

    void reset();

    bool isOverflow() const { return _overflow; }
    bool hasError() const { return _overflow; }
    size_t length() const { return _size; }
    size_t capacity() const { return _capacity; }
    const char* c_str() const;

    bool startObject();
    bool endObject();

    bool startArray();
    bool endArray();

    bool writeKey(const char* key);

    bool writeString(const char* val);
    bool writeStringEscaped(const char* str, size_t len = 0);
    bool writeRaw(const char* raw, size_t len);
    bool writeLong(long val);
    bool writeDouble(double val, int decimals = 2);
    bool writeBool(bool val);
    bool writeNull();

    bool addString(const char* key, const char* val);
    bool addRaw(const char* key, const char* rawJson, size_t rawLen);
    bool addLong(const char* key, long val);
    bool addDouble(const char* key, double val, int decimals = 2);
    bool addBool(const char* key, bool val);
    bool addNull(const char* key);
};

#endif
