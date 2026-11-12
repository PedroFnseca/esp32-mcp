#ifndef ESP32_MCP_JSON_RPC_H
#define ESP32_MCP_JSON_RPC_H

#include <Arduino.h>
#include "MCPTypes.h"
#include "json/MCPJsonScanner.h"
#include "json/MCPJsonWriter.h"

class MCPServer;

class MCPJsonRpc {
public:
    static size_t process(const char* request, size_t length, char* responseOut, size_t responseCapacity, MCPServer& server);
    static String process(const String& rawJson, MCPServer& server);
    static void buildErrorResponse(MCPJsonWriter& writer, const MCPJsonSlice& id, int code, const char* message);
};

#endif
