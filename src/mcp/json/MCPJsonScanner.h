#ifndef ESP32_MCP_JSON_SCANNER_H
#define ESP32_MCP_JSON_SCANNER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "MCPJsonConfig.h"

enum class MCPJsonType {
    Invalid = 0,
    Null,
    Boolean,
    Number,
    String,
    Object,
    Array
};

enum class MCPScanError {
    None = 0,
    BufferOverflow,
    Truncated,
    UnexpectedChar,
    DepthExceeded,
    DuplicateKey,
    IntegerOverflow,
    TrailingTrash,
    InvalidEscape,
    InvalidUnicode
};

struct MCPJsonSlice {
    const char* ptr;
    size_t len;
    MCPJsonType type;

    MCPJsonSlice() : ptr(nullptr), len(0), type(MCPJsonType::Invalid) {}
    MCPJsonSlice(const char* p, size_t l, MCPJsonType t) : ptr(p), len(l), type(t) {}

    bool isValid() const { return type != MCPJsonType::Invalid && ptr != nullptr; }
    bool isNull() const { return type == MCPJsonType::Null; }
    bool isBool() const { return type == MCPJsonType::Boolean; }
    bool isNumber() const { return type == MCPJsonType::Number; }
    bool isString() const { return type == MCPJsonType::String; }
    bool isObject() const { return type == MCPJsonType::Object; }
    bool isArray() const { return type == MCPJsonType::Array; }

    bool asBool(bool defaultValue = false) const;
    int asInt(int defaultValue = 0, bool* ok = nullptr) const;
    long asLong(long defaultValue = 0, bool* ok = nullptr) const;
    float asFloat(float defaultValue = 0.0f, bool* ok = nullptr) const;
    double asDouble(double defaultValue = 0.0, bool* ok = nullptr) const;

    bool decodeString(char* out, size_t outCapacity, size_t* outLength = nullptr) const;
    bool equalsString(const char* str) const;
};

struct MCPRpcRequestView {
    MCPJsonSlice jsonrpc;
    MCPJsonSlice id;
    MCPJsonSlice method;
    MCPJsonSlice params;
    MCPJsonSlice paramsMeta;
    MCPJsonSlice paramsName;
    MCPJsonSlice paramsArguments;
    bool isNotification;

    MCPRpcRequestView() : isNotification(false) {}
};

class MCPJsonScanner {
public:
    static bool validate(const char* json, size_t length, MCPScanError* errOut = nullptr, int maxDepth = MCP_MAX_JSON_DEPTH);
    static MCPJsonSlice findKey(const char* objJson, size_t objLength, const char* key, MCPScanError* errOut = nullptr, int maxDepth = MCP_MAX_JSON_DEPTH);
    static bool parseRpcRequest(const char* json, size_t length, MCPRpcRequestView& outView, MCPScanError* errOut = nullptr);

    static const char* skipWhitespace(const char* cur, const char* end);
    static const char* scanValue(const char* cur, const char* end, MCPJsonSlice* sliceOut, MCPScanError* errOut, int depth, int maxDepth);

private:
    static const char* scanString(const char* cur, const char* end, MCPScanError* errOut);
    static const char* scanNumber(const char* cur, const char* end, MCPScanError* errOut);
    static const char* scanObject(const char* cur, const char* end, MCPScanError* errOut, int depth, int maxDepth);
    static const char* scanArray(const char* cur, const char* end, MCPScanError* errOut, int depth, int maxDepth);
};

#endif
