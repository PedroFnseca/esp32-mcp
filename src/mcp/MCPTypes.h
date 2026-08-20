#ifndef ESP32_MCP_TYPES_H
#define ESP32_MCP_TYPES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

enum MCPErrorCode {
    MCP_RPC_PARSE_ERROR = -32700,
    MCP_RPC_INVALID_REQUEST = -32600,
    MCP_RPC_METHOD_NOT_FOUND = -32601,
    MCP_RPC_INVALID_PARAMS = -32602,
    MCP_RPC_INTERNAL_ERROR = -32603
};

class MCPResult {
public:
    bool isError;
    String content;
    String contentType;
    String errorCode;

    MCPResult() : isError(false), content(""), contentType("text"), errorCode("") {}

    static MCPResult text(const String& message) {
        MCPResult res;
        res.isError = false;
        res.content = message;
        res.contentType = "text";
        return res;
    }

    static MCPResult json(const String& jsonString) {
        MCPResult res;
        res.isError = false;
        res.content = jsonString;
        res.contentType = "text";
        return res;
    }

    static MCPResult error(const String& message) {
        MCPResult res;
        res.isError = true;
        res.content = message;
        res.contentType = "text";
        return res;
    }

    static MCPResult error(const String& code, const String& message) {
        MCPResult res;
        res.isError = true;
        res.errorCode = code;
        res.content = message;
        res.contentType = "text";
        return res;
    }
};

class MCPArguments {
private:
    JsonObjectConst _obj;

public:
    MCPArguments() {}
    MCPArguments(JsonObjectConst obj) : _obj(obj) {}

    bool has(const String& key) const {
        if (_obj.isNull()) return false;
        return !_obj[key.c_str()].isNull();
    }

    String getString(const String& key, const String& defaultValue = "") const {
        if (!has(key)) return defaultValue;
        JsonVariantConst v = _obj[key.c_str()];
        if (v.is<const char*>()) {
            return String(v.as<const char*>());
        }
        return v.as<String>();
    }

    int getInt(const String& key, int defaultValue = 0) const {
        if (!has(key)) return defaultValue;
        return _obj[key.c_str()].as<int>();
    }

    long getLong(const String& key, long defaultValue = 0) const {
        if (!has(key)) return defaultValue;
        return _obj[key.c_str()].as<long>();
    }

    float getFloat(const String& key, float defaultValue = 0.0f) const {
        if (!has(key)) return defaultValue;
        return _obj[key.c_str()].as<float>();
    }

    double getDouble(const String& key, double defaultValue = 0.0) const {
        if (!has(key)) return defaultValue;
        return _obj[key.c_str()].as<double>();
    }

    bool getBool(const String& key, bool defaultValue = false) const {
        if (!has(key)) return defaultValue;
        return _obj[key.c_str()].as<bool>();
    }

    JsonObjectConst getObject(const String& key) const {
        if (!has(key)) return JsonObjectConst();
        return _obj[key.c_str()].as<JsonObjectConst>();
    }

    JsonArrayConst getArray(const String& key) const {
        if (!has(key)) return JsonArrayConst();
        return _obj[key.c_str()].as<JsonArrayConst>();
    }

    JsonObjectConst raw() const {
        return _obj;
    }
};

#endif
