#ifndef ESP32_MCP_TOOL_REGISTRY_H
#define ESP32_MCP_TOOL_REGISTRY_H

#include <Arduino.h>
#include <vector>
#include "MCPTool.h"

class MCPToolRegistry {
private:
    std::vector<MCPTool> _tools;

public:
    MCPToolRegistry();
    ~MCPToolRegistry();

    bool registerTool(const MCPTool& tool);
    bool registerTool(const String& name, const String& description, MCPToolHandler handler);
    bool registerTool(const String& name, const String& description, const String& schemaJson, MCPToolHandler handler);

    const MCPTool* findTool(const String& name) const;
    const std::vector<MCPTool>& getTools() const;
    size_t count() const;
    void clear();
};

#endif
