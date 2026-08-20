#ifndef ESP32_MCP_SERVER_H
#define ESP32_MCP_SERVER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "MCPTypes.h"
#include "MCPTool.h"
#include "MCPToolRegistry.h"
#include "transports/MCPTransport.h"

class MCPServer {
private:
    String _name;
    String _version;
    String _protocolVersion;
    String _instructions;
    MCPToolRegistry _toolRegistry;
    MCPTransport* _transport;
    bool _ownsTransport;

public:
    MCPServer(const String& name = "ESP32-MCP", const String& version = "0.0.1");
    ~MCPServer();

    void setName(const String& name);
    const String& getName() const;

    void setVersion(const String& version);
    const String& getVersion() const;

    void setProtocolVersion(const String& protocolVersion);
    const String& getProtocolVersion() const;

    void setInstructions(const String& instructions);
    const String& getInstructions() const;

    bool tool(const String& name, const String& description, MCPToolHandler handler);
    bool tool(const String& name, const String& description, const String& schemaJson, MCPToolHandler handler);
    bool tool(const MCPTool& toolObj);

    MCPToolRegistry& getToolRegistry();
    const MCPToolRegistry& getToolRegistry() const;

    bool begin(MCPTransport* transport = nullptr);
    void handleClient();

    String processMessage(const String& message);

    bool handleServerDiscover(JsonObjectConst params, JsonObject result);
    bool handleInitialize(JsonObjectConst params, JsonObject result);
    bool handlePing(JsonObjectConst params, JsonObject result);
    bool handleToolsList(JsonObjectConst params, JsonObject result);
    bool handleToolsCall(JsonObjectConst params, JsonObject result, bool& isProtocolError, int& errorCode, String& errorMessage);
};

#endif
