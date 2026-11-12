#ifndef ESP32_MCP_TOOL_H
#define ESP32_MCP_TOOL_H

#include <Arduino.h>
#include <functional>
#include <string.h>
#include "MCPTypes.h"

typedef std::function<MCPResult(const MCPArguments&)> MCPToolHandler;

class MCPTool {
public:
    char name[32];
    char description[128];
    const char* inputSchemaJson;
    MCPToolHandler handler;

    MCPTool() : inputSchemaJson(nullptr), handler(nullptr) {
        name[0] = '\0';
        description[0] = '\0';
    }

    MCPTool(const char* toolName, const char* toolDesc, MCPToolHandler toolHandler)
        : inputSchemaJson(nullptr), handler(toolHandler) {
        strncpy(name, toolName ? toolName : "", sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        strncpy(description, toolDesc ? toolDesc : "", sizeof(description) - 1);
        description[sizeof(description) - 1] = '\0';
    }

    MCPTool(const char* toolName, const char* toolDesc, const char* schemaJson, MCPToolHandler toolHandler)
        : inputSchemaJson(schemaJson), handler(toolHandler) {
        strncpy(name, toolName ? toolName : "", sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        strncpy(description, toolDesc ? toolDesc : "", sizeof(description) - 1);
        description[sizeof(description) - 1] = '\0';
    }

    MCPTool(const String& toolName, const String& toolDesc, MCPToolHandler toolHandler)
        : MCPTool(toolName.c_str(), toolDesc.c_str(), toolHandler) {}

    MCPTool(const String& toolName, const String& toolDesc, const String& schemaJson, MCPToolHandler toolHandler)
        : inputSchemaJson(nullptr), handler(toolHandler) {
        strncpy(name, toolName.c_str(), sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        strncpy(description, toolDesc.c_str(), sizeof(description) - 1);
        description[sizeof(description) - 1] = '\0';
        inputSchemaJson = schemaJson.c_str();
    }

    bool isValid() const {
        return name[0] != '\0' && handler != nullptr;
    }
};

#endif
