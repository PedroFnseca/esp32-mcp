#ifndef ESP32_MCP_TOOL_REGISTRY_H
#define ESP32_MCP_TOOL_REGISTRY_H

#include <Arduino.h>
#include "MCPTool.h"
#include "json/MCPJsonConfig.h"

class MCPToolRegistry {
private:
    MCPTool _tools[MCP_MAX_TOOLS];
    size_t _count;

public:
    MCPToolRegistry();
    ~MCPToolRegistry();

    bool registerTool(const MCPTool& tool);
    bool registerTool(const char* name, const char* description, MCPToolHandler handler);
    bool registerTool(const char* name, const char* description, const char* schemaJson, MCPToolHandler handler);
    bool registerTool(const String& name, const String& description, MCPToolHandler handler);
    bool registerTool(const String& name, const String& description, const String& schemaJson, MCPToolHandler handler);

    const MCPTool* findTool(const char* name) const;
    const MCPTool* findTool(const String& name) const;
    const MCPTool* getTool(size_t index) const;
    size_t count() const;
    void clear();
};

#endif
