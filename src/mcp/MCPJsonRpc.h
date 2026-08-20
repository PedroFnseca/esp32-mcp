#ifndef ESP32_MCP_JSON_RPC_H
#define ESP32_MCP_JSON_RPC_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "MCPTypes.h"

class MCPServer;

class MCPJsonRpc {
public:
    static String process(const String& rawJson, MCPServer& server);
    static void buildErrorResponse(JsonDocument& doc, JsonVariantConst id, int code, const String& message);
    static void buildSuccessResponse(JsonDocument& doc, JsonVariantConst id, JsonVariantConst result);
};

#endif
