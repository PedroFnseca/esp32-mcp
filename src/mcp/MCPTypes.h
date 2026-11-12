#ifndef ESP32_MCP_TYPES_H
#define ESP32_MCP_TYPES_H

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "json/MCPJsonConfig.h"
#include "json/MCPJsonScanner.h"
#include "json/MCPJsonWriter.h"

enum MCPErrorCode {
    MCP_RPC_PARSE_ERROR = -32700,
    MCP_RPC_INVALID_REQUEST = -32600,
    MCP_RPC_METHOD_NOT_FOUND = -32601,
    MCP_RPC_INVALID_PARAMS = -32602,
    MCP_RPC_INTERNAL_ERROR = -32603
};

class MCPResult {
private:
    char _contentBuffer[256];
    const char* _contentPtr;
    char _contentType[16];
    char _errorCode[32];

public:
    bool isError;

    MCPResult() : _contentPtr(nullptr), isError(false) {
        _contentBuffer[0] = '\0';
        strncpy(_contentType, "text", sizeof(_contentType) - 1);
        _contentType[sizeof(_contentType) - 1] = '\0';
        _errorCode[0] = '\0';
    }

    const char* content() const {
        return _contentPtr ? _contentPtr : _contentBuffer;
    }

    const char* contentType() const { return _contentType; }
    const char* errorCode() const { return _errorCode; }

    MCPResult& text(const char* fmt, ...) {
        isError = false;
        strncpy(_contentType, "text", sizeof(_contentType) - 1);
        _contentType[sizeof(_contentType) - 1] = '\0';
        _errorCode[0] = '\0';

        if (fmt && strchr(fmt, '%') == nullptr) {
            _contentPtr = fmt;
            return *this;
        }

        _contentPtr = nullptr;
        va_list args;
        va_start(args, fmt);
        vsnprintf(_contentBuffer, sizeof(_contentBuffer), fmt ? fmt : "", args);
        va_end(args);
        return *this;
    }

    MCPResult& text(const String& message) {
        return text("%s", message.c_str());
    }

    MCPResult& json(const char* jsonString) {
        isError = false;
        strncpy(_contentType, "text", sizeof(_contentType) - 1);
        _contentType[sizeof(_contentType) - 1] = '\0';
        _errorCode[0] = '\0';
        _contentPtr = jsonString ? jsonString : "";
        return *this;
    }

    MCPResult& json(const String& jsonString) {
        return json(jsonString.c_str());
    }

    MCPResult& error(const char* message) {
        isError = true;
        _errorCode[0] = '\0';
        strncpy(_contentType, "text", sizeof(_contentType) - 1);
        _contentType[sizeof(_contentType) - 1] = '\0';
        _contentPtr = message ? message : "";
        return *this;
    }

    MCPResult& error(const String& message) {
        return error(message.c_str());
    }

    MCPResult& error(const char* code, const char* message) {
        isError = true;
        strncpy(_errorCode, code ? code : "", sizeof(_errorCode) - 1);
        _errorCode[sizeof(_errorCode) - 1] = '\0';
        strncpy(_contentType, "text", sizeof(_contentType) - 1);
        _contentType[sizeof(_contentType) - 1] = '\0';
        _contentPtr = message ? message : "";
        return *this;
    }

    MCPResult& error(const String& code, const String& message) {
        return error(code.c_str(), message.c_str());
    }

    static MCPResult makeText(const char* fmt, ...) {
        MCPResult res;
        if (fmt && strchr(fmt, '%') == nullptr) {
            res._contentPtr = fmt;
            return res;
        }
        res._contentPtr = nullptr;
        va_list args;
        va_start(args, fmt);
        vsnprintf(res._contentBuffer, sizeof(res._contentBuffer), fmt ? fmt : "", args);
        va_end(args);
        return res;
    }

    static MCPResult makeJson(const char* jsonStr) {
        MCPResult res;
        res.json(jsonStr);
        return res;
    }

    static MCPResult makeError(const char* msg) {
        MCPResult res;
        res.error(msg);
        return res;
    }

    static MCPResult makeError(const char* code, const char* msg) {
        MCPResult res;
        res.error(code, msg);
        return res;
    }
};

/**
 * MCPArguments wraps a slice of the incoming request JSON buffer.
 * WARNING: Values point directly to the request buffer and are ONLY valid during the tool handler invocation.
 */
class MCPArguments {
private:
    const char* _argsJson;
    size_t _argsLen;

public:
    MCPArguments() : _argsJson(nullptr), _argsLen(0) {}
    MCPArguments(const char* argsJson, size_t argsLen) : _argsJson(argsJson), _argsLen(argsLen) {}

    bool has(const char* key) const {
        if (!_argsJson || _argsLen == 0 || !key) return false;
        MCPJsonSlice slice = MCPJsonScanner::findKey(_argsJson, _argsLen, key);
        return slice.isValid() && !slice.isNull();
    }

    bool has(const String& key) const {
        return has(key.c_str());
    }

    bool getString(const char* key, char* out, size_t outCapacity) const {
        if (!has(key)) return false;
        MCPJsonSlice slice = MCPJsonScanner::findKey(_argsJson, _argsLen, key);
        return slice.decodeString(out, outCapacity);
    }

    String getString(const char* key, const String& defaultValue = "") const {
        if (!has(key)) return defaultValue;
        MCPJsonSlice slice = MCPJsonScanner::findKey(_argsJson, _argsLen, key);
        char buf[256];
        if (slice.decodeString(buf, sizeof(buf))) {
            return String(buf);
        }
        return defaultValue;
    }

    String getString(const String& key, const String& defaultValue = "") const {
        return getString(key.c_str(), defaultValue);
    }

    int getInt(const char* key, int defaultValue = 0) const {
        if (!has(key)) return defaultValue;
        MCPJsonSlice slice = MCPJsonScanner::findKey(_argsJson, _argsLen, key);
        return slice.asInt(defaultValue);
    }

    int getInt(const String& key, int defaultValue = 0) const {
        return getInt(key.c_str(), defaultValue);
    }

    long getLong(const char* key, long defaultValue = 0) const {
        if (!has(key)) return defaultValue;
        MCPJsonSlice slice = MCPJsonScanner::findKey(_argsJson, _argsLen, key);
        return slice.asLong(defaultValue);
    }

    long getLong(const String& key, long defaultValue = 0) const {
        return getLong(key.c_str(), defaultValue);
    }

    float getFloat(const char* key, float defaultValue = 0.0f) const {
        if (!has(key)) return defaultValue;
        MCPJsonSlice slice = MCPJsonScanner::findKey(_argsJson, _argsLen, key);
        return slice.asFloat(defaultValue);
    }

    float getFloat(const String& key, float defaultValue = 0.0f) const {
        return getFloat(key.c_str(), defaultValue);
    }

    double getDouble(const char* key, double defaultValue = 0.0) const {
        if (!has(key)) return defaultValue;
        MCPJsonSlice slice = MCPJsonScanner::findKey(_argsJson, _argsLen, key);
        return slice.asDouble(defaultValue);
    }

    double getDouble(const String& key, double defaultValue = 0.0) const {
        return getDouble(key.c_str(), defaultValue);
    }

    bool getBool(const char* key, bool defaultValue = false) const {
        if (!has(key)) return defaultValue;
        MCPJsonSlice slice = MCPJsonScanner::findKey(_argsJson, _argsLen, key);
        return slice.asBool(defaultValue);
    }

    bool getBool(const String& key, bool defaultValue = false) const {
        return getBool(key.c_str(), defaultValue);
    }

    MCPJsonSlice getRaw(const char* key) const {
        if (!_argsJson || _argsLen == 0 || !key) return MCPJsonSlice();
        return MCPJsonScanner::findKey(_argsJson, _argsLen, key);
    }

    MCPJsonSlice getRaw(const String& key) const {
        return getRaw(key.c_str());
    }

    const char* rawJson() const {
        return _argsJson;
    }

    size_t rawLength() const {
        return _argsLen;
    }
};

#endif
