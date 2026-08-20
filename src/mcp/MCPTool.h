#ifndef ESP32_MCP_TOOL_H
#define ESP32_MCP_TOOL_H

#include <Arduino.h>
#include <functional>
#include "MCPTypes.h"

typedef std::function<MCPResult(const MCPArguments&)> MCPToolHandler;

class MCPTool {
public:
    String name;
    String description;
    String inputSchemaJson;
    MCPToolHandler handler;

    MCPTool() : name(""), description(""), inputSchemaJson(""), handler(nullptr) {}

    MCPTool(const String& toolName, const String& toolDesc, MCPToolHandler toolHandler)
        : name(toolName), description(toolDesc), inputSchemaJson(""), handler(toolHandler) {}

    MCPTool(const String& toolName, const String& toolDesc, const String& schemaJson, MCPToolHandler toolHandler)
        : name(toolName), description(toolDesc), inputSchemaJson(schemaJson), handler(toolHandler) {}

    bool isValid() const {
        return name.length() > 0 && handler != nullptr;
    }
};

#endif
